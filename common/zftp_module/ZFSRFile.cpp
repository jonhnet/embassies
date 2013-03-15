#define _GNU_SOURCE 1
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <share.h>
#endif // _WIN32

#include "ZFSRFile.h"
#include "debug.h"
#include "xax_network_utils.h"

#ifndef S_ISREG
#define S_ISREG(mode)  (((mode) & S_IFMT) == S_IFREG)
#endif

ZFSRFile::ZFSRFile()
{
	data_file = NULL;
}

bool ZFSRFile::open(const char *path)
{
	bool result = false;
	data_file = NULL;

#ifdef _WIN32
	if ((data_file = _fsopen(path, "rb", _SH_DENYNO)) == 0 ) {
		perror("fopen_s error in ZFSRFile::open ");
		goto exit;
	}
#else 
	data_file = fopen(path, "r");
	if (data_file==NULL) {	goto exit; }
#endif

	struct stat statbuf;
	int rc;
#ifdef _WIN32
	rc = fstat(_fileno(data_file), &statbuf);
#else
	rc = fstat(fileno(data_file), &statbuf);
#endif // _WIN32
	if (rc!=0)
	{
		debug("fstat failed\n");
		goto exit;
	}
	if (!S_ISREG(statbuf.st_mode))
	{
		debug("not a regular file\n");
		goto exit;
	}

	fill_from_stat(&statbuf);
	filelen = statbuf.st_size;
	
	result = true;

exit:
	return result;
}

ZFSRFile::~ZFSRFile()
{
	if (data_file!=NULL)
	{
		fclose(data_file);
	}
}

bool ZFSRFile::is_dir(bool *isdir_out)
{
	*isdir_out = false;
	return true;
}

uint32_t ZFSRFile::get_data(uint32_t offset, uint8_t *buf, uint32_t buf_len)
{
	assert(buf);

	int rc;
	rc = fseek(data_file, offset, SEEK_SET);
	if (rc!=0) { debug("seek failed\n"); return 0; }

	uint32_t len = buf_len;
	if (offset + len > filelen)
	{
		len = filelen - offset;
		assert(len > 0);
	}
	rc = fread(buf, len, 1, data_file);
	if (rc!=1) { debug("fread failed\n"); return 0; }
	return len;
}


bool ZFSRFile::is_cacheable()
{
	return true;
}
