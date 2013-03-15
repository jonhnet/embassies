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

// Wraps a XaxVFSHandleIfc to provide file pointers and automatic
// length management for reads.

#include "XaxVFS.h"
#include "malloc_factory.h"

class XVFSHandleWrapper
{
public:
	XVFSHandleWrapper(XaxVFSHandleIfc *hdl);

	void add_ref();
	void drop_ref();

// Things that pass straight through
	void xvfs_fstat64(
		XfsErr *err, struct stat64 *buf);

	ZAS *get_zas(
		ZASChannel channel);
	
	XaxVirtualSocketIfc *as_socket();
		// returns NULL if this object doesn't support a socket ifc.

// Things that get involved in file-pointer management
	uint32_t read(
		XfsErr *err, void *dst, size_t len);

	void ftruncate(
		XfsErr *err, int64_t length);

	uint32_t write(
		XfsErr *err, const void *src, size_t len);

	uint32_t getdent64(
		XfsErr *err, struct xi_kernel_dirent64 *dirp, uint32_t byte_capacity);
		// NB pukey count-of-bytes interface
		// returns bytes used

	void llseek(
		XfsErr *err, int64_t offset, int whence, int64_t *retval);

	long status_flags;

	XaxVFSHandleIfc *get_underlying_handle() { return hdl; }

private:
	XaxVFSHandleIfc *hdl;
	int refcount;	// Handle dup()
	uint64_t file_pointer;
	uint32_t dir_pointer;

	~XVFSHandleWrapper();
	uint32_t _bound_count(uint32_t req_count);
};
