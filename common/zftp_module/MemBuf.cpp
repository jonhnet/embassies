#include "MemBuf.h"

MemBuf::MemBuf(uint8_t* buffer, uint32_t buf_len)
	: buffer(buffer),
	  buf_len(buf_len),
	  max_data_len(0)
{
}

uint8_t* MemBuf::get_data_buf()
{
	return buffer;
}

void MemBuf::write_data(ZBlockCacheRecord* block, uint32_t data_offset, uint32_t block_offset, uint32_t len)
{
	block->read(this->buffer+header_offset+data_offset, block_offset, len);
	max_data_len = data_offset + len;
}

uint32_t MemBuf::get_payload_len()
{
	return max_data_len;
}
