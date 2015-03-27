/* Copyright (c) Microsoft Corporation                                       */
/*                                                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* Licensed under the Apache License, Version 2.0 (the "License"); you may   */
/* not use this file except in compliance with the License.  You may obtain  */
/* a copy of the License at http://www.apache.org/licenses/LICENSE-2.0       */
/*                                                                           */
/* THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS     */
/* OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION      */
/* ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR   */
/* PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.                              */
/*                                                                           */
/* See the Apache Version 2.0 License for specific language governing        */
/* permissions and limitations under the License.                            */
/*                                                                           */
#pragma once

#include "malloc_factory.h"
#include "XaxVFS.h"
#include "zftp_dir_format.h"
#include "TraceCollector.h"

#define RAW_URL_SENTINEL	"raw_url"

// A helper object that understands how to decode ZFTP metadata & directory
// records.
// It's factored out because it's used for both raw ZFTP file handles
// and zarfile handles.

class ZFTPDecoder : public XaxVFSHandlePrototype {
	// A ZFTPDecoder knows how to parse out ZFTP-encoded data to satisfy
	// directory (dirent) and stat requests.
	// It's subclassed to specify how to read raw data from a specific
	// source (ZCachedFile: ZLCHandle, contiguous Zarfile: ZLCZCBHandle).
public:
	ZFTPDecoder(MallocFactory *mf, XaxVFSIfc *xvfs_for_stat, char *url_for_stat, TraceCollectorHandle* traceHandle);
	~ZFTPDecoder();

	virtual bool is_dir() = 0;
	virtual bool is_stream() { return false; }

	virtual void read(
		XfsErr *err, void *dst, size_t len, uint64_t offset);
	virtual void trace_mmap(size_t len, uint64_t offset, bool fast);
	virtual void xvfs_fstat64(
		XfsErr *err, struct stat64 *buf);
	virtual uint32_t get_dir_len(XfsErr *err);
	virtual uint32_t getdent64(
		XfsErr *err, struct xi_kernel_dirent64 *dirp,
		uint32_t item_offset, uint32_t item_count);

protected:
	virtual void _internal_read(void *buf, uint32_t count, uint32_t offset) = 0;

private:
	ZFTPDirectoryRecord *_read_record(uint32_t offset);
	uint32_t _scan_dir_index();
	void _init_dir_index();
	void _zlcfs_fill_statbuf();
	
	bool dir_loaded;
	uint32_t dirent_count;
	uint32_t *dirent_offsets;

	bool stat_loaded;

	struct stat64 statbuf;
	MallocFactory *mf;
	XaxVFSIfc *xvfs_for_stat;
	char *url_for_stat;
	TraceCollectorHandle* traceHandle;
};
