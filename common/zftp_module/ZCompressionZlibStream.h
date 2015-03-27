#pragma once

#include <zlib.h>
#include "ZCompressionIfc.h"

class ZCompressionZlibStream : public ZStreamIfc {
private:
	ZCompressionIfc::Type type;
	CompressionContext context;
	MallocFactory* mf;
	z_stream* stream;

	static uint32_t hash(void* a);
	static int32_t cmp(void* a, void* b);

public:
	ZCompressionZlibStream(ZCompressionIfc::Type type, uint32_t state_id, MallocFactory* mf, ZCompressionArgs* compression_args);
	~ZCompressionZlibStream();

	CompressionContext get_context();
	void advance_seq();

	uint32_t stretch(uint32_t payload);
	uint32_t compress(uint8_t* data_in, uint32_t len_in, uint8_t* data_out, uint32_t len_out);
	ZeroCopyBuf* decompress(uint8_t* data, uint32_t len, uint32_t expected_decompressed_size);
};

