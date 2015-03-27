#include "ZCompressionStub.h"
#include "LiteLib.h"

ZStreamIfc* ZCompressionStub::get_stream(CompressionContext context, Type type)
{
	lite_assert(false);
	return NULL;
}

CompressionContext ZCompressionStub::get_client_context()
{
	CompressionContext no_compression;
	no_compression.state_id = ZFTP_NO_COMPRESSION;
	no_compression.seq_id = 0;
	return no_compression;
}

void ZCompressionStub::reset_client_context()
{
}

CompressedZCB* ZCompressionStub::make_compressed_zcb(ZeroCopyBuf* target, AbstractSocket* socket, MallocFactory* mf, ZStreamIfc* zstream)
{
	lite_assert(false);
	return NULL;
}
