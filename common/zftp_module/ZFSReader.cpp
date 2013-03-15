#define _GNU_SOURCE 1
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "ZFSReader.h"
#include "ZFSRFile.h"
#include "ZFSRStat.h"
#include "ZFSRDir.h"
#include "ZFSReference.h"
#include "debug.h"
#include "LiteLib.h"
#include "reference_file.h"

ZFSReader::~ZFSReader()
{
}

void ZFSReader::copy(FILE *ofp, uint32_t p_offset, uint32_t len)
{
	uint8_t buf[65536];

//	uint32_t len;

//	bool rc;
//	rc = get_filelen(&len);
//	assert(rc);

	uint32_t remaining = len;
	uint32_t offset = p_offset;

	while (remaining>0)
	{
		uint32_t grab = sizeof(buf);
		if (grab > remaining)
		{
			grab = remaining;
		}
		uint32_t got = get_data(offset, buf, grab);
		assert(got==grab);
		int spewed = fwrite(buf, grab, 1, ofp);
		assert(spewed == 1);

		remaining -= grab;
		offset += grab;
	}
}
