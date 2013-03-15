#include <assert.h>
#include <stdio.h>
#include <values.h>
#include "copy_file.h"

uint32_t copy_path(const char *path, FILE *ofp)
{
	FILE *dfp = fopen(path, "r");
	uint32_t bytes = copy_file(dfp, ofp, INT_MAX);
	fclose(dfp);
	return bytes;
}

uint32_t copy_file(FILE *ifp, FILE *ofp, uint32_t len)
{
	char buf[8192];
	int bytes = 0;
	int rc;
	uint32_t total = 0;
	while (total < len)
	{
		uint32_t read_size = sizeof(buf);
		if (read_size > (len-total))
		{
			read_size = len-total;
		}
		bytes = fread(buf, 1, read_size, ifp);
		if (bytes<0)
		{
			assert(0);
		}
		if (bytes==0)
		{
			break;
		}
		rc = fwrite(buf, bytes, 1, ofp);
		assert(rc==1);
		total += bytes;
	}
	return total;
}

