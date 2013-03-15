#include "zftp_client_lib.h"

#include <alloca.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include "LiteLib.h"
#include "debug.h"
#include "debug_hash.h"
#include "malloc_trace.h"
#include "xax_network_utils.h"

#define DBG_PERF_ELIDE_HASH 0

uint8_t *
zftp_make_contiguous(ZFTPClientState *state);

void
zftp_send_request(ZFTPClientState *state, uint32_t block_num);

ZFTPClientState *
ZFTPClientState_alloc(MallocFactory *mf);

// ===========================================================================

ZFTPBlock *zftp_block_init(MallocFactory *mf, uint32_t block_index, uint8_t *buf_to_free, uint8_t *data, MallocTraceFuncs *mtf)
{
	ZFTPBlock *zb = (ZFTPBlock *) mf_malloc(mf, sizeof(ZFTPBlock));
	lite_assert(zb != NULL);
	zb->block_index = block_index;
	zb->buf_to_free = buf_to_free;
	zb->data = data;
	zb->mf = mf;
	zb->mtf = mtf;
	return zb;
}

void zftp_block_free(ZFTPBlock *zb)
{
	MallocFactory *mf = zb->mf;
	mf_free(mf, zb->buf_to_free);
	mf_free(mf, zb);
}

// ===========================================================================

hash_t zero_hash;

// ===========================================================================
ZFTPClientState *
zftp_connection_setup(ZFTPNetworkState *netstate, hash_t file_hash, const char *url_hint, MallocTraceFuncs *mtf)
{
	ZFTPClientState *state = ZFTPClientState_alloc(netstate->mf);
	lite_assert(state!=NULL);

	state->netstate = netstate;

	state->file_hash = file_hash;
	state->url_hint = mf_strdup(netstate->mf, url_hint);
	state->mtf = mtf;

	debug("zftp.get URL=%s HASH=", url_hint);
	debug_hash(file_hash);
	debug("\n");

	(state->netstate->methods.zm_get_endpoints)
		(state->netstate, &state->local_ep, &state->remote_ep);

	return state;
}

bool
zftp_get(ZFTPNetworkState *netstate, hash_t file_hash, const char *url_hint, size_t *out_size, ZFTPFileMetadata *metadata, uint8_t **contents)
{
	if (metadata!=NULL)
	{
		lite_memset(metadata, 0, sizeof(*metadata));
	}

	ZFTPClientState *state = zftp_connection_setup(netstate, file_hash, url_hint, NULL);

	state->err = 0;
	state->know_file_data = 0;
	int block_num;
	for (block_num = 0;
		state->know_file_data==0 ||
			((contents!=NULL) && block_num < state->num_blocks);
		block_num++)
	{
		ZFTPBlock *zb = zftp_fetch_block(state, block_num, false, metadata);

		if (state->err != 0)
		{
			break;
		}

		if (block_num==0)
		{
			lite_assert(state->blocks==NULL);
			state->blocks = (ZFTPBlock **)
				mf_malloc_zeroed(state->netstate->mf, state->num_blocks*sizeof(ZFTPBlock *));
		}
		lite_assert(state->blocks != NULL);
		state->blocks[block_num] = zb;
	}
	debug(" last data packet received.\n");

	if(state->err != 0)
	{
		debug("zftp."); debug_hash(state->file_hash);
		debug(" Connection terminated because of error.\n");
		ZFTPClientState_free(state);
		if (contents!=NULL)
		{
			*contents = NULL;
		}
		return false;
	}

	if (contents!=NULL)
	{
		*contents = zftp_make_contiguous(state);
	}

	*out_size = state->file_len;
	ZFTPClientState_free(state);
	return true;
}

hash_t
dev_insecure_hash_lookup(ZFTPNetworkState *netstate, char *url_hint)
{
	ZFTPClientState *state = zftp_connection_setup(netstate, zero_hash, url_hint, NULL);
	ZFTPBlock *zb = zftp_fetch_block(state, 0, true, NULL);
	if (zb!=NULL)
	{
		zftp_block_free(zb);
	}
	hash_t file_hash = state->file_hash;
	ZFTPClientState_free(state);
	return file_hash;
}

#define VALIDATE(expr)	{ if (!(expr)) { debug("zftp_fetch_block: %s fails; line %d\n", #expr, __LINE__); goto cleanup_break; } }
#define VALIDATE_CONTINUE(expr)	{ if (!(expr)) { debug("zftp_fetch_block: %s fails; line %d\n", #expr, __LINE__); goto cleanup_continue; } }

ZFTPBlock *
zftp_fetch_block(ZFTPClientState *state, off_t block_num, ZFTPFileMetadata *metadata)
{
	int consecutive_ticks = 0;
	bool retransmit = 1;

	ZFTPBlock *zb_result = NULL;
	int err_result = -1;

	uint8_t *buf = NULL;
	while (1)
	{
		if (retransmit)
		{
			zftp_send_request(state, block_num);
		}
		retransmit = 0;

		uint32_t buf_len = 65536;
		lite_assert(buf==NULL);	// else leaking
		buf = mf_malloc(state->netstate->mf, buf_len);
		UDPEndpoint sender_ep;
		int len;

		len = (state->netstate->methods.zm_recv_packet)
			(state->netstate, buf, buf_len, &sender_ep);
		//debug_print_packet(buf, len);

		//
		// looks like a timeout-timer tick
		//
		if (len==0)
		{
			// looks like a tick.
			consecutive_ticks+=1;
			if (consecutive_ticks > 1)
			{
				retransmit = 1;
			}
			goto cleanup_continue;
		}

		consecutive_ticks = 0;

		VALIDATE(len>=sizeof(ZFTPReplyHeader));
		ZFTPReplyHeader *zrh = (ZFTPReplyHeader *) buf;

		//
		// decode an error packet.
		//
		ZFTPReplyCode code = z_ntohs(zrh->code);
		if (code != zftp_reply_data)
		{
			VALIDATE(len >= sizeof(ZFTPErrorPacket));
			ZFTPErrorPacket *zep = (ZFTPErrorPacket *) buf;
			uint16_t message_len = z_ntohs(zep->message_len);
			VALIDATE(len >= sizeof(ZFTPErrorPacket) + message_len);
			char *message = zep->variable_data;
			VALIDATE(message[message_len-1] == '\0');
			debug("Error %04x, message '%s'\n", zep->header.code, message);
			goto cleanup_break;
		}

		//
		// decode a data packet.
		//
		VALIDATE(len>=sizeof(ZFTPDataPacket));

		ZFTPDataPacket *zdp = (ZFTPDataPacket *) buf;
		uint32_t file_len = z_ntohl(zdp->metadata.file_len_network_order);
		uint32_t num_blocks = compute_num_blocks(file_len);
		if (state->know_file_data)
		{
			VALIDATE(state->file_len == file_len);
			VALIDATE(state->num_blocks == num_blocks);
		}
		hash_t *pkt_file_hash = &zdp->file_hash;

		uint32_t num_merkle_records = Z_NTOHG(zdp->num_merkle_records);
		uint32_t fixed_len =
			sizeof(ZFTPDataPacket)+num_merkle_records*sizeof(ZFTPMerkleRecord);
		VALIDATE(len>=fixed_len);

		// Decode variable-length portion.
		ZFTPMerkleRecord *zmr = (ZFTPMerkleRecord *) zdp->variable_data;

		uint8_t *block_data = &buf[fixed_len];
		uint32_t block_data_len = len - fixed_len;

		lite_assert(&zmr[num_merkle_records] == block_data);

		uint32_t padding_size = Z_NTOHG(zdp->padding_size);
		block_data += padding_size;
		block_data_len -= padding_size;

		debug("fetch_block(%d). sib_hashes@%04x data@%04x (len %d)\n",
			block_num,
			((uint8_t*)siblings)-buf,
			block_data-buf,
			block_data_len);

		// check protocol constraints.
		VALIDATE(z_ntohs(zdp->hash_len) == ZFTP_HASH_LEN);
		VALIDATE(zdp->zftp_lg_block_size == ZFTP_LG_BLOCK_SIZE);
		VALIDATE(z_ntohs(zdp->zftp_tree_degree) == ZFTP_TREE_DEGREE);

		// is it the packet we're looking for?
		VALIDATE_CONTINUE(z_ntohl(zdp->first_block_num) == block_num);
		VALIDATE_CONTINUE(cmp_hash(pkt_file_hash, &state->file_hash)==0);

		// this code only looking for one block at a time.
		VALIDATE(z_ntohl(zdp->block_count) == 1);

		// verify we got all the sibling data we asked for (this code
		// asks for the entire path every time.)
		// TODO: fix here to use cached sibling hashes
		VALIDATE(zdp->first_merkle_sibling == 1);
			// 0 would mean that siblings[0] had the sibling of the root
			// node, an undefined value.

		if (block_data_len < ZFTP_BLOCK_SIZE)
		{
			// fill out block data buffer with zeros to obey consistent hash
			// rule.
			VALIDATE(fixed_len+ZFTP_BLOCK_SIZE < buf_len);
			lite_memset(block_data+block_data_len, 0, ZFTP_BLOCK_SIZE-block_data_len);
		}

		hash_t leaf_hash;
#if !DBG_PERF_ELIDE_HASH
		zhash((uint8_t *) block_data, ZFTP_BLOCK_SIZE, &leaf_hash);
#endif // !DBG_PERF_ELIDE_HASH

#ifdef DEBUG
		{
			char *tn = alloca(256);
			strcpy(tn, "/tmp/client-leaf-contents.XXXXXX");
			int tfd = mkstemp(tn);
			FILE *tf = fdopen(tfd, "w");
			fwrite((void*) block_data, ZFTP_BLOCK_SIZE, 1, tf);
			fclose(tf);
		}
		debug("leaf_hash = ");
		debug_hash(leaf_hash);
		debug("\n");
#endif

		uint16_t tree_level = compute_tree_depth(num_blocks);
		hash_t parent_hash = leaf_hash;
		uint32_t column = block_num;
		for (; tree_level>0; tree_level -= 1)
		{
			hash_t hash_pair[2];
			int sibling_idx, computed_idx;
			if (column & 1)
			{
				sibling_idx = 0;
				computed_idx = 1;
			}
			else
			{
				computed_idx = 0;
				sibling_idx = 1;
			}

			hash_pair[sibling_idx] = siblings[tree_level - zdp->first_merkle_sibling];
			hash_pair[computed_idx] = parent_hash;

#if !DBG_PERF_ELIDE_HASH
			zhash((uint8_t *) hash_pair, 2*sizeof(hash_t), &parent_hash);
#endif // !DBG_PERF_ELIDE_HASH

			debug("at level %d:\n ", tree_level);
				debug_hash(hash_pair[0]);
			debug("\n ");
				debug_hash(hash_pair[1]);
			debug("\n=");
				debug_hash(parent_hash);
			debug("\n");

			column = column >> 1;
		}

		// now fold file metadata into root of content tree to get
		// overall file_hash
		{
			ZFTPFileHashContents zfhc;
			zfhc.metadata = zdp->metadata;
			zfhc.root_hash = parent_hash;
			hash_t computed_file_hash;
#if !DBG_PERF_ELIDE_HASH
			zhash((uint8_t *) &zfhc, sizeof(zfhc), &computed_file_hash);
#endif // !DBG_PERF_ELIDE_HASH

#ifdef DEBUG
	{
		char *tn = alloca(256);
		strcpy(tn, "/tmp/client-hash.XXXXXX");
		int tfd = mkstemp(tn);
		FILE *tf = fdopen(tfd, "w");
		fwrite((void*) &zfhc, sizeof(zfhc), 1, tf);
		fclose(tf);
	}
#endif

//			debug("root hash:\n ");
//				debug_hash(parent_hash);
//				debug("\n");
//			debug("computed_file_hash:\n ");
//				debug_hash(computed_file_hash);
//				debug("\n");

#if !DBG_PERF_ELIDE_HASH
			VALIDATE(cmp_hash(&computed_file_hash, pkt_file_hash)==0);
#else
			(void) &computed_file_hash;
#endif // !DBG_PERF_ELIDE_HASH
		}

		// Yay! We now know this block is valid.
		// Hey, since we know this packet fills a valid hash tree,
		// then we know the tree's shape is correctly defined.
		state->file_len = file_len;
		state->num_blocks = num_blocks;
		state->know_file_data = 1;
		if (metadata!=NULL)
		{
			metadata->flags = zdp->metadata.flags;
			metadata->file_len_network_order = zdp->metadata.file_len_network_order;
		}

		zb_result = zftp_block_init(state->netstate->mf, block_num, buf, block_data, state->mtf);
		buf = NULL;	// don't free it here.
		err_result = 0;
cleanup_break:
		if (buf!=NULL)
		{
			mf_free(state->netstate->mf, buf);
			buf=NULL;
		}
		break;

cleanup_continue:
		if (buf!=NULL)
		{
			mf_free(state->netstate->mf, buf);
			buf=NULL;
		}
	}
	lite_assert(buf==NULL);
	state->err = err_result;
	return zb_result;
}

uint8_t *
zftp_make_contiguous(ZFTPClientState *state)
{
	lite_assert(state->know_file_data);
	int total_len = state->num_blocks * ZFTP_BLOCK_SIZE;
	uint8_t *data = mf_malloc(state->netstate->mf, total_len);
	lite_assert(data!=NULL);

	int i;
	for (i=0; i<state->num_blocks; i++)
	{
		lite_memcpy(data+i*ZFTP_BLOCK_SIZE, state->blocks[i]->data, ZFTP_BLOCK_SIZE);
	}
	
	return data;
}

void
zftp_send_request(ZFTPClientState *state, uint32_t block_num)
{
	uint16_t url_hint_len = lite_strlen(state->url_hint)+1;
	int packet_len = sizeof(ZFTPRequestPacket) +sizeof(hash_t) +url_hint_len;

	ZFTPRequestPacket *zrp = (ZFTPRequestPacket *) alloca(packet_len);
	lite_assert(zrp != NULL);
	zrp->hash_len = z_htons(ZFTP_HASH_LEN);
	zrp->url_hint_len = z_htons(url_hint_len);
	zrp->first_block_num = z_htonl(block_num);
	zrp->block_count = 1;
	zrp->first_merkle_sibling_required = 1;
	hash_t *hash_ptr = (hash_t*) zrp->variable_data;
	*hash_ptr = state->file_hash;
	char *url_hint = (char*) (&hash_ptr[1]);
	lite_memcpy(url_hint, state->url_hint, url_hint_len);
		// NB this memcpy grabs the '\0', too.

	//debug_print_packet((uint8_t*) zrp, packet_len);

	IPPacket *ipp = zftp_build_ip_packet(
		state->netstate->mf,
		(uint8_t *) zrp, packet_len, state->local_ep, state->remote_ep);

	(state->netstate->methods.zm_send_packet)
		(state->netstate, ipp);

	debug("zftp."); debug_hash(state->file_hash);
	debug(" Sent ZFTP_RRQ.\n");
}

void set_zero_hash(hash_t *hash)
{
	lite_memset(hash, 0, sizeof(*hash));
}

ZFTPClientState *
ZFTPClientState_alloc(MallocFactory *mf)
{
	set_zero_hash(&zero_hash);

	return (ZFTPClientState *) mf_malloc_zeroed(mf, sizeof(ZFTPClientState));
}

void
ZFTPClientState_free(ZFTPClientState *state)
{
	MallocFactory *mf = state->netstate->mf;
	if (state->know_file_data && state->blocks!=NULL)
	{
		int i;
		for (i=0; i<state->num_blocks; i++)
		{
			if (state->blocks[i]!=NULL)
			{
				zftp_block_free(state->blocks[i]);
			}
		}
	}
	(state->netstate->methods.zm_virtual_destructor)(state->netstate);
	mf_free(mf, state->url_hint);
	mf_free(mf, state);
}
