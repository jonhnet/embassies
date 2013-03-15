#include "ValidatedZCB.h"
#include "zemit.h"
ValidatedZCB::ValidatedZCB(ZCache* cache, ZeroCopyBuf* buffer, AbstractSocket* socket) {
	this->socket = socket;
	this->buffer = buffer;
	this->cache  = cache;

	uint32_t num_hashes = buffer->len() / ZFTP_BLOCK_SIZE-1 + 1;	// +1 for possible partial block
	this->hashes = // new hash_t[num_hashes];
		(hash_t*) mf_malloc(cache->mf, sizeof(hash_t) * num_hashes);
	memset(this->hashes, 0, sizeof(hash_t) * num_hashes);
}

ValidatedZCB::~ValidatedZCB() {
//	delete this->hashes;
	mf_free(cache->mf, this->hashes);
	socket->zc_release(buffer);
}

void ValidatedZCB::read(ZBlockCacheRecord* block, uint32_t buf_offset, 
                        uint32_t block_offset, uint32_t len) {
	if (len == ZFTP_BLOCK_SIZE) {
		lite_assert(block_offset == 0);
		lite_assert(((buf_offset-debug_offset) & (ZFTP_BLOCK_SIZE-1)) == 0);  // Should be block aligned

		uint32_t blk_index = buf_offset >> ZFTP_LG_BLOCK_SIZE;
		
		// Check whether the incoming block matches what we already have
		if (0 == memcmp(block->get_hash(), &hashes[blk_index], sizeof(hash_t))) {
			// No need to copy
			//ZLC_COMPLAIN(cache->GetZLCEmit(), "Found a matching block!\n");
			cache_hits++;
		} else {
			//ZLC_COMPLAIN(cache->GetZLCEmit(), "Stuck copying a block!\n");
			// Fill in the new data
			block->read(this->buffer->data()+buf_offset, block_offset, len);
			// Update the hash
			memcpy(&hashes[blk_index], block->get_hash(), sizeof(hash_t));
			cache_misses++;
		}
	} else {
		block->read(this->buffer->data()+buf_offset, block_offset, len);
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
