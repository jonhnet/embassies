#include "ZCompressionImpl.h"
#include "ZeroCopyBuf_Memcpy.h"
#include "ZeroCopyView.h"
#include "ZCompressionZlibStream.h"
#include "CompressedZCB.h"

ZCompressionImpl::ZCompressionImpl(MallocFactory* mf, ZCompressionArgs* compression_args)
	: mf(mf),
	  client_context_counter(11),
	  compression_args(compression_args)
{
//	hash_table_init(&streams_ht, mf, ZStream::hash, ZStream::cmp);
	cur_stream = NULL;
}

ZStreamIfc* ZCompressionImpl::get_stream(CompressionContext context, Type type)
{
	if (cur_stream!=NULL)
	{
		if (cur_stream->get_context().state_id == context.state_id)
		{
			if (cur_stream->get_context().seq_id == context.seq_id)
			{
				return cur_stream;
			}
#if 0
			fprintf(stderr, "Bummer: CompressionContext desynchronization. For state_id %d, I had seq %d, they asked for seq %d.\n",
				context.state_id,
				cur_stream->get_context().seq_id,
				context.seq_id);
#endif
		}

		delete cur_stream;
		cur_stream = NULL;
	}
	lite_assert(cur_stream==NULL);
	cur_stream = new ZCompressionZlibStream(type, context.state_id, mf, compression_args);
	return cur_stream;
}

CompressionContext ZCompressionImpl::get_client_context()
{
	if (!compression_args->use_compression)
	{
		CompressionContext context = { ZFTP_NO_COMPRESSION, 0 };
		return context;
	}

	if (cur_stream==NULL)
	{
		cur_stream = new ZCompressionZlibStream(DECOMPRESS, client_context_counter, mf, compression_args);
	}
	return cur_stream->get_context();
}

void ZCompressionImpl::reset_client_context()
{
	if (cur_stream!=NULL)
	{
		delete cur_stream;
		cur_stream = NULL;
		client_context_counter += 1;
	}
}

CompressedZCB* ZCompressionImpl::make_compressed_zcb(
	ZeroCopyBuf* target, AbstractSocket* socket, MallocFactory* mf, ZStreamIfc* zstream)
{
	return new CompressedZCB(target, socket, mf, zstream);
}
