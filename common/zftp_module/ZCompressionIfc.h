#pragma once

#include "ZStreamIfc.h"

// Interface shields elf_loader, which isn't compression-capable,
// from needing zlib or related files.

class CompressedZCB;
class AbstractSocket;

class ZCompressionIfc {
public:
	enum Type { COMPRESS, DECOMPRESS, KEY };

	virtual ZStreamIfc* get_stream(CompressionContext context, Type type) = 0;
		// create stream if context unrecognized or mismatched
	virtual CompressionContext get_client_context() = 0;
	virtual void reset_client_context() = 0;

	virtual CompressedZCB* make_compressed_zcb(ZeroCopyBuf* target, AbstractSocket* socket, MallocFactory* mf, ZStreamIfc* zstream) = 0;
};
