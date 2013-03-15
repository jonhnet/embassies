#include "MemBuf.h"

MemBuf::MemBuf(uint8_t* buffer, uint32_t buf_len) {
	this->buffer = buffer;
	this->buf_len = buf_len;
}

void MemBuf::read(ZBlockCacheRecord* block, uint32_t buf_offset, 
                        uint32_t block_offset, uint32_t len) {
	block->read(this->buffer+buf_offset, block_offset, len);
}
