#ifdef _WIN32
#include <malloc.h>
#else
#include <alloca.h>
#endif // _WIN32

#include "xax_network_utils.h"
#include "zftp_util.h"
#include "zlc_util.h"
#include "ZCachedFile.h"
#include "ZFileRequest.h"
#include "ZFileServer.h"
#include "ZBlockCache.h"
#include "ZBlockCacheRecord.h"
#include "ZCache.h"
#include "tree_math.h"
#include "ErrorFileResult.h"
#include "ValidFileResult.h"
#include "debug.h"

#include "ZFileDB.h"
#include "ZBlockDB.h"

ZCachedFile::ZCachedFile(ZCache *zcache, const hash_t *file_hash)
{
	_ctor_init(zcache, file_hash);

	this->origin_mode = false;
}

void ZCachedFile::_ctor_init(ZCache *zcache, const hash_t *file_hash)
{
	_common_init(zcache);
	this->ze = zcache->GetZLCEmit();
	this->file_hash = *file_hash;
	lite_memset(&this->metadata, 0, sizeof(this->metadata));
	this->metadata_state = v_unknown;
	this->status = zcs_ignorant;
	this->progress_counter = 0;
	this->zstats = new ZStats(ze);
}

void ZCachedFile::_common_init(ZCache *zcache)
{
	this->zcache = zcache;
	this->zstats = NULL;
	if (zcache!=NULL)
	{
		this->mf = zcache->mf;
		linked_list_init(&this->waiters, mf);
		hash_table_init(&this->parents_of_tentative_nodes, mf,
			ValidatedMerkleRecord::__hash__, ValidatedMerkleRecord::__cmp__);
	}
	else
	{
		this->mf = NULL;
		this->zstats=NULL;
	}
	this->tree = NULL;
	this->requesting_data_mode = false;

	mutex = zcache->sf->new_mutex(false);
}

ZCachedFile::ZCachedFile(const hash_t *file_hash)
{
	// key constructor.
	this->ze = NULL;
	this->zcache = NULL;
	this->tree = NULL;
	this->file_hash = *file_hash;
	this->origin_mode = false;
}

ZCachedFile::~ZCachedFile()
{
	if (this->zcache!=NULL)
	{
		hash_table_free(&this->parents_of_tentative_nodes);
	}
	if (this->tree!=NULL)	// not a zombie key object
	{
		delete mutex;
		mf_free(mf, this->tree);
	}
}

	// ca & cb should be siblings, but may be provided in either order.
void ZCachedFile::_hash_children(uint32_t ca, uint32_t cb, hash_t *hash_out)
{
	uint32_t left_sib, right_sib;
	if (ca<cb)
	{
		left_sib = ca;
		right_sib = cb;
	}
	else
	{
		left_sib = cb;
		right_sib = ca;
	}
	lite_assert((left_sib&1)==1);
	lite_assert(left_sib+1 == right_sib);

	hash_t child_hashes[2];
	child_hashes[0] = this->tree[left_sib].hash;
	child_hashes[1] = this->tree[right_sib].hash;
	zhash((uint8_t*) child_hashes, sizeof(child_hashes), hash_out);
}

void ZCachedFile::_track_parent_of_tentative_nodes(ValidatedMerkleRecord *parent)
{
	// this parent isn't eligible to hash until both its siblings are
	// at least tentative.
	if (tree[_left_child(parent->location())].state == v_unknown)
	{
		return;
	}
	if (tree[_right_child(parent->location())].state == v_unknown)
	{
		return;
	}
	hash_table_union(&this->parents_of_tentative_nodes, parent);
}

void ZCachedFile::_assert_root_validated()
{
	lite_assert(tree[0].state == v_validated);
	// files we're loading incrementally from an upstream server
	// should have their root node validated *first*,
	// not along the way..
}

void ZCachedFile::_mark_node_tentative(uint32_t location)
{
	ValidatedMerkleRecord *vmr = &this->tree[location];
	lite_assert(vmr->state!=v_validated);
	vmr->state = v_tentative;

	if (location>0)
	{
		uint32_t parent_loc = _parent_location(location);
		ValidatedMerkleRecord *parent = &this->tree[parent_loc];
		_track_parent_of_tentative_nodes(parent);
		ZLC_CHATTY(ze, "zcachedfile_mark_node_tentative(%d): marking %d\n",, location, parent_loc);

		uint32_t sibling_loc = _sibling_location(location);
		ValidatedMerkleRecord *sibling = &this->tree[sibling_loc];
		if (parent->state == v_unknown && sibling->state != v_unknown)
		{
			// parent could be hashed!
			_hash_children(location, sibling_loc, &parent->hash);
			_mark_node_tentative(parent_loc);
		}
	}
	else
	{
		_assert_root_validated();
	}
}

bool ZCachedFile::_load_hash(ZFTPMerkleRecord *record)
{
	uint32_t location = Z_NTOHG(record->tree_location);
	ZLC_VERIFY(ze, location < this->tree_size);

	ValidatedMerkleRecord *vmr;
	vmr = &this->tree[location];
	if (vmr->state == v_validated)
	{
		// already have this hash.
		ZLC_VERIFY(ze, cmp_hash(&vmr->hash, &record->hash)==0);

		// (this is just a sanity check; if it fails; no harm.)
		ZLC_CHATTY(ze, "got a hash we didn't need %d\n",, location);
		return false;
	}

	vmr->hash = record->hash;
	_mark_node_tentative(location);
	return true;

fail:
	return false;
}

void ZCachedFile::_load_block(
	uint32_t block_num, uint8_t *in_data, uint32_t valid_data_len)
{
	// this method used on the ZOriginFile load path.
	ZBlockCacheRecord *block = new ZBlockCacheRecord(zcache, in_data, valid_data_len);
	hash_t hash = *block->get_hash();
		// stash hash before calling insert(), which may delete block
		// (if it's a duplicate)
	zcache->insert_block(block);

	uint32_t leaf_location = _tree_index(this->tree_depth, block_num, this->tree_size);
	ValidatedMerkleRecord *vmr = &this->tree[leaf_location];

	if (vmr->state == v_validated)
	{
		if (cmp_hash(&hash, &vmr->hash)!=0)
		{
			ZLC_COMPLAIN(ze, "block hash mismatches validated leaf hash. Block stashed but useless.\n");
		}
	}
	else
	{
		vmr->hash = hash;
		_mark_node_on_block_load(leaf_location);

		char hashbuf[66];
		ZLC_CHATTY(ze,
			"_load_block(block %d, %d bytes) -> tentative hash %s\n",,
			block_num, valid_data_len, debug_hash_to_str(hashbuf, &hash));
	}
}

void ZCachedFile::_mark_node_on_block_load(uint32_t location)
{
	_mark_node_tentative(location);
}

hash_t* ZCachedFile::find_block_hash(uint32_t block_num, ValidityState required_validity) {
	uint32_t leaf_loc = _tree_index(this->tree_depth, block_num, this->tree_size);
	ValidatedMerkleRecord *vmr = &this->tree[leaf_loc];
	if (vmr->state < required_validity)
	{
		return NULL;
	}

	return &vmr->hash;
}


ZBlockCacheRecord *ZCachedFile::_lookup_block(uint32_t block_num, ValidityState required_validity)
{
	hash_t* block_hash = find_block_hash(block_num, required_validity);
	if (block_hash == NULL) {
		return NULL;
	}

	ZBlockCacheRecord *block = zcache->block_cache->lookup(block_hash);

	if (block==NULL)
	{
		block = new ZBlockCacheRecord(zcache, block_hash);
		zcache->insert_block(block);
	}
	return block;
}

void ZCachedFile::_error_reply(ZFileRequest *req, ZFTPReplyCode code, const char *msg)
{
	req->deliver_result(new ErrorFileResult(this, code, msg));
	zstats->reset();
}

DataRange ZCachedFile::get_file_data_range()
{
	lite_assert(metadata_state == v_validated);
	return DataRange(0, get_filelen());
}

ValidatedMerkleRecord *ZCachedFile::get_validated_merkle_record(
	uint32_t location)
{
	ValidatedMerkleRecord *vmr = &tree[location];
	lite_assert(vmr->state == v_validated);
	return vmr;
}

void ZCachedFile::write_blocks_to_buf(Buf* buf, uint32_t packet_data_offset, uint32_t offset, uint32_t size)
{
	lite_assert(offset+size <= get_filelen());

	uint32_t first_block = offset >> ZFTP_LG_BLOCK_SIZE;

	uint32_t blk = first_block;
	uint32_t file_offset = offset;
	uint32_t buf_off = 0;
#define DBG_ASSERTS 1
#if DBG_ASSERTS
	uint32_t dbg_file_last_ended = offset;
	//uint8_t *dbg_buf_last_ended = buf;
#endif // DBG_ASSERTS

	buf->resetDebugStats();

	while (buf_off < size)
	{
		ZBlockCacheRecord *block = _lookup_block(blk, v_validated);
		lite_assert(block!=NULL);
		lite_assert(block->is_valid());		// We always fill in blocks before calling read

		uint32_t blk_start = blk << ZFTP_LG_BLOCK_SIZE;
		uint32_t blk_off = 0;
		if (file_offset > blk_start)
		{
			blk_off = file_offset - blk_start;
		}
		uint32_t blk_end_request = file_offset+size - blk_start;
		uint32_t blk_len = min(
			blk_end_request - blk_off,			// end of user request
			ZFTP_BLOCK_SIZE - blk_off);	// end of block
		buf->write_data(block, buf_off + packet_data_offset, blk_off, blk_len);
		
#if DBG_ASSERTS
		{
			// offset into file is consistent with expectations
			lite_assert(blk_start+blk_off==dbg_file_last_ended);
			dbg_file_last_ended=blk_start + blk_off + blk_len;
			// offset into user buffer is consistent with expectations
			//lite_assert(buf+buf_off==dbg_buf_last_ended);
			//dbg_buf_last_ended = buf+buf_off+blk_len;
		}
#endif // DBG_ASSERTS

		blk += 1;
		buf_off += blk_len;
	}
#if DBG_ASSERTS
	// offset into file is consistent with expectations
	lite_assert(dbg_file_last_ended==offset+size);
	// offset into user buffer is consistent with expectations
	//lite_assert(dbg_buf_last_ended==buf+size);
#endif // DBG_ASSERTS
//	fprintf(stderr, "Wrote %x bytes\n", size);
	// buf->printDebugStats();
	// 		Dude, that's not a safe assumption!
}

void ZCachedFile::_issue_server_request(
	DataRange missing_set,
	ZFileRequest *waiting_req)
{
	if (cmp_hash(waiting_req->get_hash(), &zcache->zero_hash)==0)
	{
		_error_reply(waiting_req, zftp_reply_error_yeah_that_is_zero,
			"Request asks to invert the zero hash.");
	}
	else if (zcache->zfile_client!=NULL)
	{
		OutboundRequest *obr;
		zcache->get_compression()->reset_client_context();	// TODO DEBUG
		CompressionContext compression_context =
			zcache->get_compression()->get_client_context();
		if (this->metadata_state != v_validated)
		{
			obr = new OutboundRequest(
				mf,
				compression_context,
				waiting_req->get_hash(),
				waiting_req->url_hint,
				1,
				0,
				DataRange());
			obr->set_location(0, 0);	// get the root hash
		}
		else
		{
			lite_assert(missing_set.size() > 0);
			RequestBuilder builder(this, zcache->sf, missing_set, zcache->zfile_client->get_max_payload());
			obr = builder.as_outbound_request(compression_context);
			requesting_data_mode = builder.requested_any_data();
		}
		zcache->zfile_client->issue_request(obr);
		//ZLC_CHATTY(ze, "insert waiter %08x\n",, (uint32_t) waiting_req);
		linked_list_insert_tail(&this->waiters, waiting_req);
	}
	else if (waiting_req->url_hint!=NULL && zcache->has_origins())
	{
		ZLC_CHATTY(ze, "Probing origins\n");
		bool hash_match = zcache->probe_origins_for_hash(waiting_req->url_hint, &this->file_hash);
		if (!hash_match)
		{
			_error_reply(waiting_req, zftp_reply_error_incorrect_hash_for_url,
				"Provided url_hint doesn't map to requested hash.");
		}
		// TODO wait, should we now wake up the waiting_req?
	}
	else
	{
		_error_reply(waiting_req, zftp_reply_hint_required,
			"No back-end origin. This message probably not helpful.");
	}
	// TODO leak: we aren't ever removing the requests from the waiting queue.
}

uint32_t ZCachedFile::round_up_to_block(uint32_t pos)
{
	uint32_t down = ((pos-1)&(~(ZFTP_BLOCK_SIZE-1)));
	if (down+ZFTP_BLOCK_SIZE < down)
	{
		// overflow -- caller sent an address near 0xfffffff. Just leave
		// this pointing at penultimate possible block.
	}
	else
	{
		down += ZFTP_BLOCK_SIZE;
	}
	return down;
}

DataRange ZCachedFile::_compute_missing_set(DataRange requested_range)
{
	uint32_t max_payload = 1;
	if (zcache->zfile_client!=NULL)	// TODO this is a factoring nightmare
	{
		max_payload = zcache->zfile_client->get_max_payload();
	}

	// NB if anything from requested_range is missing, this returns
	// a nonempty range; however, it's not guaranteed to return the
	// entire set of missing stuff, because there's no reason to
	// enumerate it all every time if we can only request a block or
	// fraction thereof each time.
	// TODO even still, it's O(n^2), since we rescan from the beginning
	// of the requested range each time. Consider keeping a "known-valid"
	// we can use to fast-forward.

	// Now if the original request doesn't start or end on a block boundary,
	// we want to round it out. Otherwise, we'll request a fraction
	// of a block, receive it, and have an invalid (partly-filled)
	// data block, with which we can't resolve the original request.

	DataRange rounded_set = DataRange(
		(requested_range.start() & (~(ZFTP_BLOCK_SIZE-1))),
		round_up_to_block(requested_range.end()));

	// But then prune back down to the file length, so we don't request
	// zeros beyond file len (which we do know how to pad out to a valid
	// block ourselves).

	DataRange eligible_set = rounded_set.intersect(get_file_data_range());

	ZLC_CHATTY(ze, "eligible_set is [%d,%d)\n",,
		eligible_set.start(), eligible_set.end());

	DataRange missing_set, *dbg_missing_set = &missing_set;
	(void) dbg_missing_set;
	uint32_t start = eligible_set.start();
	uint32_t dbg_block_count = 0;
	while (start < eligible_set.end())
	{
		dbg_block_count += 1;
		uint32_t blk = start>>ZFTP_LG_BLOCK_SIZE;
		uint32_t start_blk = (blk)<<ZFTP_LG_BLOCK_SIZE;
		uint32_t end_blk = (blk+1)<<ZFTP_LG_BLOCK_SIZE;
		uint32_t end = min(end_blk, eligible_set.end());
		if (blk >= this->num_blocks)
		{
			ZLC_CHATTY(ze, "byte %d / blk %d (%d) beyond file_len\n",,
				start, blk, start_blk);
			// request is beyond my file_len; stop here.
			break;
		}
		ZBlockCacheRecord *block = _lookup_block(blk, v_validated);
		if (block == NULL)
		{
			bool worked = missing_set.maybe_accumulate(start, end);
			if (!worked)
			{
				// See note in corresponding block below.
				ZLC_CHATTY(ze, "Noncontiguous missing block; stopping short.\n");
				break;
			}
			ZLC_CHATTY(ze, "byte %d / blk %d (%d) missing entirely\n",,
				start, blk, start_blk);
		}
		else if (!block->is_valid())
		{
			DataRange blk_range = block->get_missing_range();
			DataRange shifted_range(
				start_blk+blk_range.start(),
				start_blk+blk_range.end());
			DataRange pruned_range = shifted_range.intersect(eligible_set);
			bool worked = missing_set.maybe_accumulate(pruned_range);
			if (!worked)
			{
				// This can happen for a file with the same block occuring
				// twice (as in test_rf:57), where getting part of the
				// first block also gets us part of the second block,
				// leaving nonadjacent gaps.
				// It could also happen if the block appears in another
				// file in a simultaneous request.
				// In any case, stop here with what we can represent, fill
				// the hole, and continue on later.
				ZLC_CHATTY(ze, "Noncontiguous missing sets; stopping short.\n");
				break;
			}
			ZLC_CHATTY(ze, "byte %d / blk %d (%d) has hash, missing data\n",,
				start, blk, start_blk);
		}
		else
		{
			ZLC_CHATTY(ze, "byte %d / blk %d (%d) is valid\n",,
				start, blk, start_blk);
		}

		// Don't bother enumerating more stuff than we'll be able to get
		// back in a reply.
		uint32_t approx_payload_size;
		if (requesting_data_mode)
		{
			// last time we did this calculation, we way over-estimated,
			// because our request was limited by data size.
			// Don't bother doing so much enumerating; else this ends
			// up behaving O(n^2).
			approx_payload_size = missing_set.size();
		}
		else
		{
			// Last time we did this calculation, we under-estimated,
			// because we ended up only asking for hashes.
			// Take the time to enumerate more stuff so that
			// we don't need O(n) round trips to learn all the leaf hashes.
			approx_payload_size =
				(missing_set.size() >> ZFTP_LG_BLOCK_SIZE)
				* sizeof(ZFTPMerkleRecord);
		}
		if (approx_payload_size >= max_payload)
		{
			break;
		}
		start = end_blk;
	}
	ZLC_CHATTY(ze, "_compute_missing_set: %s mode, visited %d blocks\n",,
		requesting_data_mode ? "data" : "hash",
		dbg_block_count);
	return missing_set;
}

void ZCachedFile::_consider_request_nolock(ZFileRequest *req)
{
	DataRange missing_set;

	if (this->status == zcs_ignorant)
	{
		// hey, wait, we could check the disk!
		ZCachedFile *zcf_image = NULL;
		zcf_image = zcache->retrieve_file(&file_hash);
		if (zcf_image!=NULL)
		{
			_install_image(zcf_image, zcs_clean);
		}
		//...and continue.
	}

	bool missing_info = true;
	if (this->metadata_state == v_validated)
	{
		missing_set = _compute_missing_set(req->get_data_range());
		if (missing_set.is_empty())
		{
			missing_info = false;
		}
	}

	if (missing_info)
	{
		// there is a run of missing blocks, [first_missing,blk).
		// Form a request.
		_issue_server_request(missing_set, req);
		ZLC_CHATTY(ze,
			"zcachedfile_consider_request: Need bytes %d--%d (%d/%d)\n",,
			missing_set.start(), missing_set.end(),
			missing_set.size(), req->get_data_range().size());
		goto defer;
	}

	// all blocks present and available. Yay. Form a reply.
	ZLC_CHATTY(ze, "zcachedfile_consider_request: replying.\n");
	req->deliver_result(new ValidFileResult(this));
		// req passed back off to ZFileServer, who shall free it.

defer:
	ZLC_CHATTY(ze, "zcachedfile_consider_request: defer\n");
}

void ZCachedFile::consider_request(ZFileRequest *req)
{
	mutex->lock();
	_consider_request_nolock(req);
	mutex->unlock();
}

void ZCachedFile::withdraw_request(ZFileRequest *req)
{
	mutex->lock();
	if (linked_list_find(&this->waiters, req)!=NULL)
	{
		linked_list_remove(&this->waiters, req);
	}
	else
	{
		// already withdrawn, apparently
	}
	mutex->unlock();
}

void ZCachedFile::_signal_waiters()
{
#if 0	// from the time I discovered that the warm cache case
// was slow because it was doing 34 synchronous round-trips.
	PerfMeasureIfc* pmi = zcache->get_perf_measure_ifc();
	if (pmi!=NULL)
	{
		pmi->mark_time("_signal_waiters");
	}
#endif

	// snarf out the waiters list...
	LinkedList waiters;
	lite_memcpy(&waiters, &this->waiters, sizeof(LinkedList));
	linked_list_init(&this->waiters, this->mf);

	ZLC_CHATTY(ze, "zcachedfile_signal_waiters: enumerating %d waiters.\n",, waiters.count);

	// and reconsider each in turn. Those that don't get satisfied
	// will end up back on the waiters list.
	while (waiters.count>0)
	{
		ZFileRequest *request =
			(ZFileRequest*) linked_list_remove_head(&waiters);
//		ZLC_CHATTY(ze, "zcachedfile_signal_waiters: here's one %08x\n",, (uint32_t) request);
		_consider_request_nolock(request);
	}
}

ZFTPMerkleRecord *ZCachedFile::_find_root_merkle_record_idx(ZFileReplyFromServer *reply)
{
	for (uint32_t i=0; i<reply->get_num_merkle_records(); i++)
	{
		ZFTPMerkleRecord *zmr = reply->get_merkle_record(i);
		if (Z_NTOHG(zmr->tree_location)==0)
		{
			return zmr;
		}
	}
	return NULL;
}

ZCachedFile::InstallResult ZCachedFile::_install_metadata(ZFileReplyFromServer *reply)
{
	if (this->metadata_state == v_validated)
	{
		return NO_EFFECT;
	}

	ZFTPMerkleRecord *root_zmr = _find_root_merkle_record_idx(reply);
	if (root_zmr==NULL) { return FAIL; }

	hash_t *root_hash = &root_zmr->hash;
	hash_t computed_file_hash = _compute_file_hash(reply->get_metadata(), root_hash);

	if (cmp_hash(&computed_file_hash, &this->file_hash)!=0)
	{
		ZLC_COMPLAIN(ze, "root hash,metadata!=file hash\n");
		return FAIL;
	}
	// Hooray! Metadata & root_hash are now known to be valid.

	lite_memcpy(&this->metadata, reply->get_metadata(), sizeof(this->metadata));

	_configure_from_metadata();

	this->tree[0].hash = *root_hash;
	this->tree[0].state = v_validated;
	this->tree[0].set_location(0);

	return NEW_DATA;
}

hash_t ZCachedFile::_compute_file_hash(ZFTPFileMetadata *metadata, hash_t *root_hash)
{
	int zfm_size = sizeof(ZFTPFileMetadata)+sizeof(hash_t);
	ZFTPFileMetadata *zfm = (ZFTPFileMetadata *)
		alloca(sizeof(ZFTPFileMetadata)+sizeof(hash_t)); 
	*zfm = *metadata;
	hash_t *root_hash_ptr = (hash_t*) (&zfm[1]);
	*root_hash_ptr = *root_hash;

	hash_t computed_file_hash;
	zhash((uint8_t*) zfm, zfm_size, &computed_file_hash);

	return computed_file_hash;
}
	
void ZCachedFile::_configure_from_metadata()
{
	this->metadata_state = v_validated;

	this->file_len = Z_NTOHG(this->metadata.file_len_network_order);
	this->num_blocks = compute_num_blocks(this->file_len);
	this->tree_depth = compute_tree_depth(this->num_blocks);
	this->tree_size = compute_tree_size(this->tree_depth);

	this->tree = (ValidatedMerkleRecord*) mf_malloc(mf, sizeof(ValidatedMerkleRecord)*this->tree_size);
	lite_memset(this->tree, 0, sizeof(ValidatedMerkleRecord)*this->tree_size);
	for (uint32_t i=0; i<this->tree_size; i++)
	{
		this->tree[i].set_location(i);
	}
}

void _visit_parent(void *v_zcf, void *datum)
{
	ZCachedFile *zcf = (ZCachedFile *) v_zcf;
	ValidatedMerkleRecord *vmr = (ValidatedMerkleRecord *) datum;
	zcf->sorted_parents[zcf->sorted_index++] = vmr;
}

void ZCachedFile::_mark_hash_validated(ValidatedMerkleRecord *vmr)
{
	vmr->state = v_validated;
	int level, col;
	_tree_unindex(vmr->location(), &level, &col);
	lite_assert (level<=this->tree_depth);
	if (level<this->tree_depth)
	{
		return;	//not a leaf
	}
	if (!(col < (int) this->num_blocks))
	{
		return;	// out of block range
	}
}

ZCachedFile::InstallResult ZCachedFile::_validate_children(ValidatedMerkleRecord *parent)
{
	uint32_t parent_loc = parent->location();
	uint32_t left_sib = _left_child(parent_loc);
	uint32_t right_sib = left_sib+1;
	lite_assert(right_sib < this->tree_size);
	lite_assert(this->tree[left_sib].state != v_unknown);
	lite_assert(this->tree[right_sib].state != v_unknown);

	hash_t parent_hash_computed;
	_hash_children(left_sib, right_sib, &parent_hash_computed);

	ZLC_CHATTY(ze, "_validate_children hashes\n");
	char hashbuf[66];
	ZLC_CHATTY(ze, "  left  @%d %s\n",,
		left_sib,
		debug_hash_to_str(hashbuf, &tree[left_sib].hash));
	ZLC_CHATTY(ze, "  right @%d %s\n",,
		right_sib,
		debug_hash_to_str(hashbuf, &tree[right_sib].hash));
	ZLC_CHATTY(ze, "  computed %s\n",,
		debug_hash_to_str(hashbuf, &parent_hash_computed));
	ZLC_CHATTY(ze, "  expected %s\n",,
		debug_hash_to_str(hashbuf, &parent->hash));

	if (cmp_hash(&parent->hash, &parent_hash_computed)==0)
	{
		_mark_hash_validated(&this->tree[left_sib]);
		_mark_hash_validated(&this->tree[right_sib]);
		ZLC_CHATTY(ze, "zcachedfile_validate_children: %d,%d=>%d\n",,
			left_sib, right_sib, parent_loc);
		return NEW_DATA;
	}
	else
	{
		ZLC_COMPLAIN(ze, "zcachedfile_validate_children: A hash validation step failed.\n");
		return FAIL;
	}
}

ZCachedFile::InstallResult ZCachedFile::_validate_tentative_hashes()
{
	InstallResult result = NO_EFFECT;
	uint32_t num_parents = parents_of_tentative_nodes.count;

	sorted_parents = (ValidatedMerkleRecord **)
		alloca(sizeof(ValidatedMerkleRecord *) * num_parents);
	sorted_index = 0;

	hash_table_remove_every(&parents_of_tentative_nodes, _visit_parent, this);
	lite_assert(sorted_index == num_parents);
	lite_assert(parents_of_tentative_nodes.count == 0);

	ZLC_CHATTY(ze, "zcachedfile_validate_tentative_hashes: enumerating %d\n",,
		num_parents);
	for (uint32_t i=0; i<num_parents; i++)
	{
		ZLC_CHATTY(ze, "zcachedfile_validate_tentative_hashes: %d/%d is block %d\n",,
			i, num_parents, sorted_parents[i]->location());
		InstallResult ir = _validate_children(sorted_parents[i]);
		if (ir==FAIL)
		{
			result = FAIL;
		}
		else
		{
			result = NEW_DATA;
		}
	}

	this->sorted_parents = NULL;
	return result;
}

bool ZCachedFile::install_data(ZFileReplyFromServer *reply)
{
	bool caused_change = false;
	uint32_t next_start_byte = reply->get_data_start();
	while (next_start_byte < reply->get_data_end())
	{
		uint32_t next_start_block = (next_start_byte >> ZFTP_LG_BLOCK_SIZE);
		uint32_t block_start_byte = next_start_block << ZFTP_LG_BLOCK_SIZE;

		if (next_start_block >= num_blocks)
		{
			ZLC_COMPLAIN(ze, "Received data beyond last block.\n");
			lite_assert(false);
		}

		uint32_t block_end_byte = block_start_byte + ZFTP_BLOCK_SIZE;
		uint32_t next_end_byte = min(block_end_byte, reply->get_data_end());
		ZBlockCacheRecord *block_record = _lookup_block(next_start_block, v_tentative);
		lite_assert(block_record!=NULL);
		caused_change |= block_record->receive_data(
			next_start_byte - block_start_byte,
			next_end_byte - block_start_byte,
			reply,
			next_start_byte - reply->get_data_start());
			// may generate an implied tentative hash value at the leaf,
			// and thence up the tree.
		ZLC_CHATTY(ze, "  install data %d -- %d\n",, next_start_byte, next_end_byte);

		if (next_start_block == num_blocks-1 && next_end_byte==get_filelen())
		{
			// we won't see any more data for this block -- better fill
			// it out so it acts valid.
			block_record->complete_with_zeros();
		}

		next_start_byte = next_end_byte;
	}
	return caused_change;
}

bool ZCachedFile::install_merkle_records(ZFileReplyFromServer *reply)
{
	bool caused_change = false;
	for (uint32_t mri=0; mri<reply->get_num_merkle_records(); mri++)
	{
		ZFTPMerkleRecord *zmr = reply->get_merkle_record(mri);
		uint32_t location = Z_NTOHG(zmr->tree_location);

		ZLC_CHATTY(ze, "  install record at loc %d\n",, location);

		ValidatedMerkleRecord *vmr = &this->tree[location];
		if (vmr->state == v_validated)
		{
			lite_assert(cmp_hash(&vmr->hash, &zmr->hash)==0);
				// new tentative hash mismatches existing
		}
		else if (vmr->state == v_tentative)
		{
			if (cmp_hash(&vmr->hash, &zmr->hash)!=0)
			{
				ZLC_COMPLAIN(ze, "New tentative hash doesn't match old one.\n");
			}
		}
		else if (vmr->state == v_unknown)
		{
			// Yay! New data!
			vmr->hash = zmr->hash;
			_mark_node_tentative(location);
			caused_change = true;
		}
		else
		{
			lite_assert(false);
			// case shouldn't happen outside ZOriginFile, right?
		}
	}
	return caused_change;
}
void ZCachedFile::install_reply(ZFileReplyFromServer *reply)
{
	bool caused_change = false;
	mutex->lock();

	zstats->install_reply(reply);

	ZLC_CHATTY(ze, "II== ===== Install reply\n");
	ZLC_CHATTY(ze, "II== Install metadata\n");
	InstallResult ir;
	ir = _install_metadata(reply);
	if (ir==FAIL)
	{
		ZLC_COMPLAIN(ze, "Couldn't validate metadata, so couldn't allocate data structures.\n");
		goto fail;
	}
	else if (ir==NEW_DATA)
	{
		caused_change = true;
	}

	// Load tentative hashes from the merkle records
	// We do these first, because if we're configured to get partial
	// blocks, we need to know what hash to (tentatively) name them by
	// until the whole block is fetched.
	ZLC_CHATTY(ze, "II== install_merkle_records\n");
	caused_change |= install_merkle_records(reply);

	// Now load all the data.
	ZLC_CHATTY(ze, "II== install_data\n");
	caused_change |= install_data(reply);

	ZLC_CHATTY(ze, "II== _validate_tentative_hashes\n");
	ir = _validate_tentative_hashes();
	if (ir==NEW_DATA)
	{
		// Wait, why could validating tentative hashes create new data
		// if nothing else in the packet did already?
		lite_assert(caused_change);

		caused_change = true;
	}
	ZLC_CHATTY(ze, "II== Done.\n");

	if (caused_change)
	{
		progress_counter += 1;
		_signal_waiters();
		_update_status(zcs_dirty);
	}
	else
	{
		ZLC_TERSE(ze, "install_reply: packet told us nothing new; skipping reevaluation.\n");
	}

fail:
	mutex->unlock();
}

void ZCachedFile::_update_status(ZCachedStatus new_status)
{
	if (new_status==zcs_dirty)
	{
		zcache->store_file(this);
		this->status = zcs_clean;
	}
	else
	{
		this->status = new_status;
	}
}

void ZCachedFile::_install_into(ZCachedFile *target)
{
	// this is the origin file, target is the cached file
	target->_install_image(this, zcs_dirty);
}

void ZCachedFile::_install_image(ZCachedFile *image, ZCachedStatus new_status)
{
	// An image is a ZCF that came from an origin or a FileDB.
	// It's not a value in the zcache file_ht, and this object
	// is. Some clients may be waiting on this object. So we
	// install the asynchronously-acquired knowledge from the
	// image here, and then signal the waiters.

	if (metadata_state==v_validated)
	{
		// oh, we already knew all this stuff, and someone just happened
		// to look up a file with identical contents. Nothing (new) to record.
		lite_assert(cmp_hash(&this->file_hash, &image->file_hash)==0);
		lite_assert(this->tree_size == image->tree_size);
		lite_assert(lite_memcmp(this->tree, image->tree, sizeof(ValidatedMerkleRecord)*tree_size)==0);
		return;
	}

	lite_assert(metadata_state==v_unknown);	// TODO handle cases where
		// this already has partial knowledge.
	lite_assert(tree==NULL);

	file_hash = image->file_hash;
	metadata = image->metadata;
	_configure_from_metadata();
	lite_memcpy(tree, image->tree, sizeof(ValidatedMerkleRecord)*this->tree_size);

	_update_status(new_status);

	_signal_waiters();
}

uint32_t ZCachedFile::__hash__(const void *a)
{
	ZCachedFile *zcf = (ZCachedFile *) a;
	return ((uint32_t*) &zcf->file_hash)[0];
}

int ZCachedFile::__cmp__(const void *a, const void *b)
{
	ZCachedFile *zcf_a = (ZCachedFile *) a;
	ZCachedFile *zcf_b = (ZCachedFile *) b;
	return cmp_hash(&zcf_a->file_hash, &zcf_b->file_hash);
}

//////////////////////////////////////////////////////////////////////////////

#if ZLC_USE_PERSISTENT_STORAGE
void ZCachedFile::marshal(ZDBDatum *v)
{
	lite_assert(metadata_state == v_validated);
		// shouldn't be storing skeleton objects in persistent store.
	v->write_buf(&file_hash, sizeof(file_hash));
	v->write_buf(&metadata, sizeof(metadata));
	v->write_buf(tree, sizeof(ValidatedMerkleRecord)*tree_size);
}

ZCachedFile::ZCachedFile(ZCache *zcache, ZDBDatum *v)
{	// load from disk ctor
	_common_init(zcache);

	v->read_buf(&file_hash, sizeof(file_hash));
	v->read_buf(&metadata, sizeof(metadata));
	metadata_state = v_validated;
	_configure_from_metadata();
	v->read_buf(tree, sizeof(ValidatedMerkleRecord)*tree_size);

	this->origin_mode = false;
	this->status = zcs_clean;
}
#endif // ZLC_USE_PERSISTENT_STORAGE

bool ZCachedFile::is_dir()
{
	lite_assert(metadata_state == v_validated);
	return (Z_NTOHG(metadata.flags) & ZFTP_METADATA_FLAG_ISDIR)!=0;
}

SendBufferFactory *ZCachedFile::get_send_buffer_factory()
{
	return zcache->get_send_buffer_factory();
}
