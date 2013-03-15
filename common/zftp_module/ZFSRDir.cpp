#define _GNU_SOURCE 1
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <Windows.h>
#endif // _WIN32

#include "ZFSRDir.h"
#include "debug.h"
#include "zftp_dir_format.h"
#include "xax_network_utils.h"

ZFSRDir::ZFSRDir()
{
	precomputed_contents = NULL;
}

ZFSRDir::~ZFSRDir()
{
	if (precomputed_contents != NULL)
	{
		free(precomputed_contents);
	}
}

bool ZFSRDir::open()
{
	int rc;

	precomputed_contents = NULL;
	precomputed_contents_len = 0;
	scan_len = 0;
	rc = scan();
	if (!rc) { goto fail; }
	
	precomputed_contents_len = scan_len;
	precomputed_contents = (uint8_t*) malloc(precomputed_contents_len);
	if (precomputed_contents==NULL) { goto fail; }

	bool rcb;
	rcb = fetch_stat();
	if (!rcb)
	{
		return false;
	}
#ifndef _WIN32
#else
	// TODO this needs to move into ZFSRDir_Win's fetch_stat method
	// Windows' stat is very literal in its interpretation of filenames
	// (adding a trailing slash causes it to fail), so open file and retrieve info in the Windows way
	FILETIME modified_time;
	BY_HANDLE_FILE_INFORMATION fileInfo;
	HANDLE file = CreateFile(path, GENERIC_READ, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		// invalid path
		goto fail;
	}
	if (GetFileTime(file, NULL, NULL, &modified_time) ) {
		this->mtime = (time_t) ((modified_time.dwHighDateTime << sizeof(DWORD)) & modified_time.dwLowDateTime);
		this->zum.mtime = this->mtime;
	} else {
		// Failed to retrieve time
		CloseHandle(file);
		goto fail;
	}
	if (GetFileInformationByHandle(file, &fileInfo)) {
		this->zum.dev = fileInfo.dwVolumeSerialNumber;
		this->zum.ino = (fileInfo.nFileIndexHigh << sizeof(DWORD)) | fileInfo.nFileIndexLow;
	} else {
		// Failed to get file info
		CloseHandle(file);
		goto fail;
	}

	CloseHandle(file);

	/** This appears to fail the sensible-path test too
	FILE* file = NULL;
	if ((file = _fsopen(path, "r", _SH_DENYNO)) == 0 ) {
		perror("fopen_s error in ZFSRDir::open ");
		rc = -1;
	} else {
		rc = fstat(_fileno(file), &statbuf);
		fclose(file);
	}
	*/
#endif // _WIN32

	scan_len = 0;
	rc = scan();
	if (!rc) { goto fail; }
	if (scan_len != precomputed_contents_len) { goto fail; }

	filelen = precomputed_contents_len;

	return true;

fail:
	return false;
}

void ZFSRDir::scan_add_one(HandyDirRec *hdr)
{
	uint32_t new_len = scan_len + hdr->get_record_size();
	if (precomputed_contents!=NULL)
	{
		assert(new_len <= precomputed_contents_len);
		hdr->write(precomputed_contents+scan_len);
	}
	scan_len=new_len;
	delete hdr;
}

bool ZFSRDir::is_dir(bool *isdir_out)
{
	*isdir_out = true;
	return true;
}

uint32_t ZFSRDir::get_data(uint32_t offset, uint8_t *buf, uint32_t size)
{
	uint32_t req_len = size;
	uint32_t len_available = filelen - offset;
	if (req_len > len_available)
	{
		req_len = len_available;
	}
	memcpy(buf, precomputed_contents + offset, req_len);
	return req_len;
}

bool ZFSRDir::is_cacheable()
{
	return true;
}
