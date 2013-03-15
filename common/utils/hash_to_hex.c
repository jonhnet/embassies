#include "hash_to_hex.h"

void hash_to_hex(hash_t hash, char *out_str)
{
	static const char *NYBBLE = "0123456789abcdef";
	char *p = out_str;
	uint8_t* hb = (uint8_t*) &hash;
	// TODO more buffer-overflowy goodness.
	uint32_t i;
	for (i=0; i<sizeof(hash_t); i++)
	{
		*p++ = NYBBLE[(hb[i]>>4) & 0xf];
		*p++ = NYBBLE[(hb[i]   ) & 0xf];
	}
	*p = '\0';
}
