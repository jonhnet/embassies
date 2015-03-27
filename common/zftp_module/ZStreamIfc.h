#pragma once

#include <zlib.h>
#include "pal_abi/pal_types.h"
#include "ZeroCopyBuf.h"
#include "malloc_factory.h"
#include "zftp_protocol.h"
#include "ZLCArgs.h"

class ZStreamIfc {
public:
	virtual ~ZStreamIfc() {}

	virtual CompressionContext get_context() = 0;
	virtual void advance_seq() = 0;
	virtual uint32_t stretch(uint32_t payload) = 0;
	virtual uint32_t compress(uint8_t* data_in, uint32_t len_in, uint8_t* data_out, uint32_t len_out) = 0;
	virtual ZeroCopyBuf* decompress(uint8_t* data, uint32_t len, uint32_t expected_decompressed_size) = 0;
};

