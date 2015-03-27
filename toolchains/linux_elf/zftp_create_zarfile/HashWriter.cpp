#include "HashWriter.h"
#include "crypto.h"
#include "hash_table.h"

HashWriter::HashWriter()
	: hash_value(0)
{
}

void HashWriter::write(uint8_t* data, int len)
{
	hash_t h;
	zhash(data, len, &h);
	hash_value ^= hash_buf(&h, sizeof(h));
}

uint32_t HashWriter::get_hash()
{
	return hash_value;
}
