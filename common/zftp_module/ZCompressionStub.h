#pragma once

#include "ZCompressionIfc.h"

class ZCompressionStub : public ZCompressionIfc {
public:
	virtual ZStreamIfc* get_stream(CompressionContext context, Type type);
	virtual CompressionContext get_client_context();
	virtual void reset_client_context();
	virtual CompressedZCB* make_compressed_zcb(ZeroCopyBuf* target, AbstractSocket* socket, MallocFactory* mf, ZStreamIfc* zstream);
};
