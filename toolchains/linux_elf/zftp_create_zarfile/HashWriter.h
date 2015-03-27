#pragma once
#include "WriterIfc.h"

// NB this doesn't produce zhash(chunk), as that would require
// exposing an incremental interface to zhash. Instead, I just
// hash a bunch of pieces and xor them together. Caller doesn't
// care; it doesn't have to be a known hash value, just stable and
// hashy.

class HashWriter : public WriterIfc
{
private:
	uint32_t hash_value;
public:
	HashWriter();
	void write(uint8_t* data, int len);
	uint32_t get_hash();
};

