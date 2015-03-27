#pragma once

#include <stdint.h>

class WriterIfc
{
public:
	virtual void write(uint8_t* data, int len) = 0;
};

