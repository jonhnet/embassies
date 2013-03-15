#include "zlc_util.h"

char *debug_hash_to_str(char *buf, hash_t *hash)
{
	char *bp = buf;
	uint8_t *raw = hash->bytes;
	for (uint32_t i = 0; i < sizeof(hash_t); i++)
	{
		hexbyte(bp, raw[i]&0xff);
		bp+=2;
	}
	*bp = '\0';
	return buf;
}


