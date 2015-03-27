#include "CompressedZCB.h"
#if DEBUG_COMPRESSION
#include "ZCompressionZlibStream.h"
#endif // DEBUG_COMPRESSION

CompressedZCB::CompressedZCB(ZeroCopyBuf* target, AbstractSocket* socket, MallocFactory* mf, ZStreamIfc* zstream)
	: buffer(target),
	  socket(socket),
	  mf(mf),
	  zstream(zstream),
	  writing_data(false),
	  virtual_data_offset(0),
	  compressed_offset(0)
{
	// TODO should take some sort of lock on zstream, or have some
	// other argument why noone will concurrently change compression state!
}

CompressedZCB::~CompressedZCB()
{
	zstream->advance_seq();
	socket->zc_release(buffer);
}

uint8_t* CompressedZCB::get_data_buf()
{
	return buffer->data();
}

uint8_t* CompressedZCB::write_header(uint32_t len)
{
	lite_assert(!writing_data);
	return Buf::write_header(len);
}

void CompressedZCB::_close_header()
{
	if (!writing_data)
	{
		compressed_offset = header_offset;
		writing_data = true;
	}
}

#if DEBUG_COMPRESSION
void CompressedZCB::start_data(uint32_t start, uint32_t size)
{
	snprintf(uncompressed_fn, sizeof(uncompressed_fn),
		"/tmp/debug_uncompressed_%08x_%08x", start, size);
	debug_uncompressed_fp = fopen(uncompressed_fn, "wb");

	snprintf(compressed_fn, sizeof(compressed_fn),
		"/tmp/debug_compressed_%08x_%08x", start, size);
	debug_compressed_fp = fopen(compressed_fn, "wb");
	debug_expected_size = size;
}

void CompressedZCB::end_data()
{
	fclose(debug_compressed_fp);
	fclose(debug_uncompressed_fp);

	if (debug_expected_size <= 0) { return; }

	int rc;
	FILE* c_fp = fopen(compressed_fn, "rb");
	fseek(c_fp, 0, SEEK_END);
	int c_len = ftell(c_fp);
	fseek(c_fp, 0, SEEK_SET);
	uint8_t* c_data = (uint8_t*) mf_malloc(mf, c_len);
	rc = fread(c_data, c_len, 1, c_fp);
	lite_assert(rc==1);
	fclose(c_fp);

	ZCompressionZlibStream* dbg_stream = new ZCompressionZlibStream(
		ZCompression::DECOMPRESS, 87, mf, NULL);
	ZeroCopyBuf* zcb = dbg_stream->decompress(c_data, c_len, debug_expected_size+1);
	lite_assert(zcb->len() == debug_expected_size);
//	lite_assert(lite_memcmp(zcb->data(), u_data, u_len)==0);
	delete dbg_stream;
	mf_free(mf, c_data);
}
#endif // DEBUG_COMPRESSION

void CompressedZCB::write_data(ZBlockCacheRecord* block, uint32_t data_offset, uint32_t block_offset, uint32_t len)
{
#if DEBUG_COMPRESSION
//	start_data(block_offset, len);
#endif // DEBUG_COMPRESSION
	_close_header();
	lite_assert(data_offset == virtual_data_offset);
		// only used to be sure caller is writing data in order.

	// TODO memcpy-happy. But this is the compression path, so meh.
	uint8_t* tbuf = (uint8_t*) mf_malloc(mf, len);
	block->read(tbuf, block_offset, len);
	uint32_t size = zstream->compress(tbuf, len,
		buffer->data()+compressed_offset, buffer->len()-compressed_offset);

#if DEBUG_COMPRESSION
{
	int rc;
	rc = fwrite(tbuf, len, 1, debug_uncompressed_fp);
	lite_assert(rc==1);
	rc = fwrite(buffer->data()+compressed_offset, size, 1, debug_compressed_fp);
	lite_assert(rc==1);
}
#endif // DEBUG_COMPRESSION
		
#if DEBUG_COMPRESSION
//	end_data();
#endif // DEBUG_COMPRESSION

	mf_free(mf, tbuf);
	compressed_offset += size;

	virtual_data_offset += len;

}

uint32_t CompressedZCB::get_payload_len()
{
	_close_header();
	return compressed_offset - header_offset;
}

CompressionContext CompressedZCB::get_compression_context()
{
	return zstream->get_context();
}

