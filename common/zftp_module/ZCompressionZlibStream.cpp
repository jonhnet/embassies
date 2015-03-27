#include "ZCompressionZlibStream.h"
#include "ZeroCopyBuf_Memcpy.h"
#include "ZeroCopyView.h"

static void* z_mf_alloc(void* opaque, uInt items, uInt size)
{
	return mf_malloc((MallocFactory*) opaque, items*size);
}

static void z_mf_free(void* opaque, voidpf address)
{
	mf_free((MallocFactory*) opaque, address);
}

ZCompressionZlibStream::ZCompressionZlibStream(ZCompressionIfc::Type type, uint32_t state_id, MallocFactory* mf, ZCompressionArgs* compression_args)
	: type(type),
	  mf(mf),
	  stream(NULL)
{
	context.state_id = state_id;
	context.seq_id = 0;

	if (type!=ZCompressionIfc::KEY)
	{
		stream = new z_stream;
		stream->zalloc = z_mf_alloc;
		stream->zfree = z_mf_free;
		stream->opaque = mf;
		if (type==ZCompressionIfc::COMPRESS)
		{
//			deflateInit(stream, 1);	// very compressy?
			deflateInit2(stream,
				compression_args->zlib_compression_level,
				Z_DEFLATED,
				15 /* windowBits default according to zlib.h */,
				8 /* memLevel default according to zlib.h */,
				compression_args->zlib_compression_strategy);
		}
		else if (type==ZCompressionIfc::DECOMPRESS)
		{
			inflateInit(stream);
		}
		else
		{
			lite_assert(false);
		}
	}
}

ZCompressionZlibStream::~ZCompressionZlibStream()
{
	if (stream!=NULL)
	{
		int rc;
		if (type==ZCompressionIfc::COMPRESS)
		{
			rc = deflateEnd(stream);
			// rc==Z_DATA_ERROR (-3) here means we didn't allocate enough
			// space on the output, and some data's still stuck in the
			// compressor's internal buffers.
//			lite_assert(rc == Z_OK);
			// Well, not exactly: when we use Z_SYNC_FLUSH, we leave behind
			// some bits of an empty block in the compressor stream, and
			// that's okay. There's no truly clean way to detect that
			// real bits are stuck in it.
		}
		else if (type==ZCompressionIfc::DECOMPRESS)
		{
			rc = inflateEnd(stream);
			lite_assert(rc == Z_OK);
		}
		else
		{
			lite_assert(false);
		}
		delete stream;
	}
}

CompressionContext ZCompressionZlibStream::get_context()
{
	return context;
}

void ZCompressionZlibStream::advance_seq()
{
	context.seq_id += 1;
}

uint32_t ZCompressionZlibStream::compress(uint8_t* data_in, uint32_t len_in, uint8_t* data_out, uint32_t len_out)
{
	lite_assert(type==ZCompressionIfc::COMPRESS);

	stream->next_out = data_out;
	stream->avail_out = len_out;
	stream->next_in = data_in;
	stream->avail_in = len_in;

	int rc = deflate(stream, Z_SYNC_FLUSH);
	lite_assert(rc == Z_OK);
	uint32_t transformed_bytes = len_out - stream->avail_out;
	lite_assert(stream->avail_out > 0);
		// else zlib may be holding some bits behind.
	lite_assert(stream->avail_in==0);
#if 0
	fprintf(stderr,
		"ZCompressionZlibStream(%d,%d) 0x%08x compresses %x bytes to %x payload\n",
		context.state_id, context.seq_id,
		(uint32_t) this, len_in, transformed_bytes);
#endif
	return transformed_bytes;
}

ZeroCopyBuf* ZCompressionZlibStream::decompress(uint8_t* data, uint32_t len, uint32_t expected_decompressed_size)
{
	lite_assert(type==ZCompressionIfc::DECOMPRESS);

	ZeroCopyBuf* zcb = new ZeroCopyBuf_Memcpy(mf, expected_decompressed_size);

	stream->next_out = zcb->data();
	stream->avail_out = zcb->len();
	stream->next_in = data;
	stream->avail_in = len;
	int rc = inflate(stream, Z_SYNC_FLUSH);
	lite_assert(rc == Z_OK);
	uint32_t transformed_bytes = zcb->len() - stream->avail_out;
#if 0
	fprintf(stderr,
		"ZCompressionZlibStream(%d,%d) 0x%08x decompresses %x bytes to %x payload\n",
		context.state_id, context.seq_id,
		(uint32_t) this, len, transformed_bytes);
#endif
	return new ZeroCopyView(zcb, true, 0, transformed_bytes);
}

uint32_t ZCompressionZlibStream::stretch(uint32_t payload)
{
#if 0
	// Per http://www.zlib.net/zlib_tech.html, the output may be
	// a little bigger than the input:
	// 5 bytes per 16K block plus 6 bytes.
	// I tossed in 30 more because I'm feeling paranoid.
	// Any overestimate here only affects allocation, of course.
	// (The client is still responsible to leave enough headroom below the
	// MTU for the packet to actually *get there*, though.)
	return payload + ((payload >> 14)+1)*5+6 + 30;
#endif // or we could do in the right way.
	lite_assert(type == ZCompressionIfc::COMPRESS);
	return deflateBound(stream, payload);
}
