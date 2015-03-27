#include "Buf.h"

Buf::Buf()
	: header_offset(0)
{
}

uint8_t* Buf::write_header(uint32_t len)
{
	uint8_t* ptr = this->get_data_buf() + header_offset;
	header_offset += len;
	return ptr;
}

uint32_t Buf::get_packet_len()
{
	return header_offset + get_payload_len();
}

CompressionContext Buf::get_compression_context()
{
	CompressionContext null_compression_context;
	null_compression_context.state_id = ZFTP_NO_COMPRESSION;
	null_compression_context.seq_id = 0;
	return null_compression_context;
}

void Buf::recycle()
{
	lite_assert(false);
}
