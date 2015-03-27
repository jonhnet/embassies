#include "StdioWriter.h"
#include "LiteLib.h"

StdioWriter::StdioWriter(FILE* fp)
	: fp(fp)
{
}

void StdioWriter::write(uint8_t* data, int len)
{
	int rc;
	rc = fwrite(data, len, 1, fp);
	lite_assert(rc==1);
}
