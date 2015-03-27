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

#include "XaxVFS.h"

class FilterVFS;

class FilterVFSHandle : public XaxVFSHandleIfc {
public:
	FilterVFSHandle(XaxVFSHandleIfc *base_handle);
	~FilterVFSHandle();

	virtual bool is_stream() { return false; }
	virtual uint64_t get_file_len();

	virtual void ftruncate(
		XfsErr *err, int64_t length);
	virtual void xvfs_fstat64(
		XfsErr *err, struct stat64 *buf);
	virtual uint32_t read(
		XfsErr *err, void *dst, size_t len);
	virtual void read(
		XfsErr *err, void *dst, size_t len, uint64_t offset);
	virtual void* fast_mmap(size_t len, uint64_t offset);
	virtual void trace_mmap(size_t len, uint64_t offset, bool fast);
	virtual void write(
		XfsErr *err, const void *src, size_t len, uint64_t offset);
	virtual uint32_t get_dir_len(XfsErr *err);
	virtual uint32_t getdent64(
		XfsErr *err, struct xi_kernel_dirent64 *dirp,
		uint32_t item_offset, uint32_t item_count);
	virtual ZAS *get_zas(
		ZASChannel channel);
	virtual XaxVirtualSocketIfc *as_socket() { return NULL; }

protected:
	XaxVFSHandleIfc *base_handle;
};

class FilterVFS : public XaxVFSPrototype {
public:
	FilterVFS(XaxVFSIfc *base_vfs);
	~FilterVFS();

	// implementor supplies open(), which calls specific handle ctor

	virtual void mkdir(
		XfsErr *err, XfsPath *path, const char *new_name, mode_t mode);

	virtual void stat64(
		XfsErr *err,
		XfsPath *path, struct stat64 *buf);

protected:
	XaxVFSIfc *base_vfs;
};
