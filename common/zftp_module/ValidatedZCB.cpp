#include "ValidatedZCB.h"
#include "zemit.h"
#include "cheesy_snprintf.h"
#include "zlc_util.h"

ValidatedZCB::ValidatedZCB(ZCache* cache, ZeroCopyBuf* buffer, AbstractSocket* socket)
	: buffer(buffer),
	  socket(socket),
	  cache(cache)
{
	uint32_t num_hashes = buffer->len() / ZFTP_BLOCK_SIZE-1 + 1;	// +1 for possible partial block
	this->hashes = // new hash_t[num_hashes];
		(hash_t*) mf_malloc(cache->mf, sizeof(hash_t) * num_hashes);
	memset(this->hashes, 0, sizeof(hash_t) * num_hashes);
	resetDebugStats();
}

ValidatedZCB::~ValidatedZCB() {
//	delete this->hashes;
	mf_free(cache->mf, this->hashes);
	socket->zc_release(buffer);
}

uint8_t* ValidatedZCB::get_data_buf()
{
	return buffer->data();
}

void ValidatedZCB::write_data(ZBlockCacheRecord* block, uint32_t data_offset, uint32_t block_offset, uint32_t len)
{
	if (len == ZFTP_BLOCK_SIZE)
	{
		lite_assert(block_offset == 0);
		lite_assert((data_offset & (ZFTP_BLOCK_SIZE-1)) == 0);
		// Should be block aligned

		uint32_t blk_index = data_offset >> ZFTP_LG_BLOCK_SIZE;
			// NB this is count of blocks *into the data packet*, not
			// into the file we're reading. We use that in the next line
			// to index our metadata to see what block is already there,
			// to avoid a memcpy.
		
		// Check whether the incoming block matches what we already have
		if (0 == memcmp(block->get_hash(), &hashes[blk_index], sizeof(hash_t))) {
			// No need to copy
			//ZLC_COMPLAIN(cache->GetZLCEmit(), "Found a matching block!\n");
			cache_hits++;
		} else {
#if 0
			char oldhashbuf[sizeof(hash_t)*2+1];
			debug_hash_to_str(oldhashbuf, (hash_t*) &hashes[blk_index]);
			char newhashbuf[sizeof(hash_t)*2+1];
			debug_hash_to_str(newhashbuf, (hash_t*) block->get_hash());
			char msgbuf[1000];
			cheesy_snprintf(msgbuf, sizeof(msgbuf), "vzcb block mismatch: old %s new %s\n", oldhashbuf, newhashbuf);
			ZLC_COMPLAIN(cache->GetZLCEmit(), msgbuf);
#endif

			//ZLC_COMPLAIN(cache->GetZLCEmit(), "Stuck copying a block!\n");
			// Fill in the new data
			block->read(this->buffer->data()+header_offset+data_offset, block_offset, len);
			// Update the hash
			memcpy(&hashes[blk_index], block->get_hash(), sizeof(hash_t));
			cache_misses++;
		}
	} else {
		block->read(this->buffer->data()+header_offset+data_offset, block_offset, len);
	}
}

void ValidatedZCB::resetDebugStats() {
	cache_hits = 0;
	cache_misses = 0;
}

void ValidatedZCB::printDebugStats() {
	ZLC_COMPLAIN(cache->GetZLCEmit(), "Hit the cache %d times, missed %d times\n",,
			cache_hits, cache_misses);
}

uint32_t ValidatedZCB::get_payload_len()
{
	// assume we end up filling buffer, since we don't compress.
	return buffer->len() - header_offset;
}

void ValidatedZCB::recycle()
{
	header_offset = 0;
}
