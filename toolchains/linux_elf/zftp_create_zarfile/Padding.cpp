#include <malloc.h>
#include <string.h>

#include "Padding.h"
#include "LiteLib.h"

Padding::Padding(uint32_t size)
{
	this->size = size;
}

uint32_t Padding::get_size()
{
	return size;
}

const char* Padding::get_type()
{
	return "Padding";
}

void Padding::emit(FILE *fp)
{
	char *buf = (char*) malloc(size);
	memset(buf, 0, size);
	int rc = fwrite(buf, size, 1, fp);
	lite_assert(rc==1);
	free(buf);
}
