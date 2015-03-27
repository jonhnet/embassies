#pragma once

#if DEBUG_COMPRESSION
#include <stdio.h>
#endif // DEBUG_COMPRESSION

#include "Buf.h"
#include "ZCompressionIfc.h"
#include "SocketFactory.h"

class CompressedZCB
	: public Buf
#if DEBUG_COMPRESSION
	, DebugCompression
#endif
{
private:
	ZeroCopyBuf* buffer;
	AbstractSocket* socket;
	MallocFactory* mf;
	ZStreamIfc* zstream;

	bool writing_data;
	uint32_t virtual_data_offset;	// count of uncompressed data bytes written
	uint32_t compressed_offset;		// count of compressed bytes stored

	void _close_header();

protected:
	uint8_t* get_data_buf();

public:
	CompressedZCB(ZeroCopyBuf* target, AbstractSocket* socket, MallocFactory* mf, ZStreamIfc* zstream);
	~CompressedZCB();

	virtual uint8_t* write_header(uint32_t len);
	virtual void write_data(ZBlockCacheRecord* block, uint32_t data_offset, uint32_t block_offset, uint32_t len);
	virtual uint32_t get_payload_len();

	virtual CompressionContext get_compression_context();

#if DEBUG_COMPRESSION
	virtual DebugCompression* debug_compression()
		{ return (DebugCompression*) this; }
	virtual void start_data(uint32_t start, uint32_t size);
	virtual void end_data();
private:
	char compressed_fn[80];
	char uncompressed_fn[80];
	FILE* debug_uncompressed_fp;
	FILE* debug_compressed_fp;
	uint32_t debug_expected_size;
#endif //DEBUG_COMPRESSION
};
