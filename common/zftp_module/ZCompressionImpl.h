#pragma once

#include <zlib.h>
#include "pal_abi/pal_types.h"
#include "ZeroCopyBuf.h"
#include "malloc_factory.h"
#include "zftp_protocol.h"
#include "ZLCArgs.h"
#include "ZCompressionIfc.h"

class ZCompressionImpl : public ZCompressionIfc {
private:
	//HashTable streams_ht;
	MallocFactory* mf;
	ZStreamIfc *cur_stream;	// 1-bucket table. For easy eviction. :v)
	int client_context_counter;
	ZCompressionArgs* compression_args;

public:
	ZCompressionImpl(MallocFactory* mf, ZCompressionArgs* compression_args);
	virtual ZStreamIfc* get_stream(CompressionContext context, Type type);
		// create stream if context unrecognized or mismatched
	virtual CompressionContext get_client_context();
	virtual void reset_client_context();
	virtual CompressedZCB* make_compressed_zcb(ZeroCopyBuf* target, AbstractSocket* socket, MallocFactory* mf, ZStreamIfc* zstream);

	// TODO evict accumulated streams
};
