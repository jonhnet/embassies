#pragma once

#include "pal_abi/pal_types.h"

#error DEAD CODE

class ClientCompressionContext
{
private:
	uint32_t seq;

public:
	ClientCompressionContext();

	uint32_t get_state();
	uint32_t get_seq();
};
