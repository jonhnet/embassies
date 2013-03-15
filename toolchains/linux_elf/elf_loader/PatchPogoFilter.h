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

// A filter fs that automatically detects and patches ELF files with
// pogo sections. This is how we connect syscall refs from binary-rewritten
// libraries back to our emualtion layer.

#include "malloc_factory.h"
#include "FilterVFS.h"
#include "elfobj.h"
#include "x86assembly.h"

#define POGO_PATCH_SIZE \
	(sizeof(x86_push_immediate)+sizeof(uint32_t)+sizeof(x86_ret))

class PatchPogoVFS;

class PatchPogoHandle : public FilterVFSHandle {
public:
	PatchPogoHandle(XaxVFSHandleIfc *base_handle, Elf32_Shdr *pogo_shdr);
	~PatchPogoHandle();

	virtual void read(
		XfsErr *err, void *dst, size_t len, uint64_t offset);

private:
	void copy_in(
		void *dst_buf, uint32_t dst_offset, uint32_t dst_len,
		void *src_buf, uint32_t src_offset, uint32_t src_len);

	uint8_t patch_bytes[POGO_PATCH_SIZE];
	uint32_t patch_offset;
};

//////////////////////////////////////////////////////////////////////////////

class PatchPogoVFS : public FilterVFS {
public:
	PatchPogoVFS(XaxVFSIfc *base_vfs, MallocFactory *mf);

	virtual XaxVFSHandleIfc *open(
		XfsErr *err, XfsPath *path, int oflag, XVOpenHooks *open_hooks = NULL);

private:
	MallocFactory *mf;
};

