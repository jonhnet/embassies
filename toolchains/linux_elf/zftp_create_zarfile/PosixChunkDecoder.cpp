#include "PosixChunkDecoder.h"

void PosixChunkDecoder::underlying_zarfile_read(
	void *out_buf, uint32_t len, uint32_t offset)
{
	int rc;
	rc = fseek(ifp, offset, SEEK_SET);
	lite_assert(rc==0);

	rc = fread(out_buf, len, 1, ifp);
	lite_assert(rc==1);
}
