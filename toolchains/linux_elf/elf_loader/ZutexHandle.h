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
#include "pal_abi/pal_abi.h"
#include "LiteLib.h"

// Implementation of xe_open_zutex_as_fd(zutex).
// This wraps the zutex in a ZAS, so the app can select() or poll() it.
//
// VERY IMPORTANT NOTE if you want your process to ever go to sleep again:
// To reset the ZAS, just read a dummy buffer off the fd.

class ZutexHandle : public XaxVFSHandlePrototype
{
public:
	ZutexHandle(uint32_t *zutex);

	virtual bool is_stream() { return true; }
	virtual uint64_t get_file_len() { lite_assert(false); return 0; }

	virtual uint32_t read(
		XfsErr *err, void *dst, size_t len);
	virtual void write(
		XfsErr *err, const void *src, size_t len, uint64_t offset)
		{ lite_assert(false); }
	void xvfs_fstat64(
		XfsErr *err, struct stat64 *buf)
		{ lite_assert(false); }

	virtual ZAS *get_zas(
		ZASChannel channel);

private:
	uint32_t zutex_value;
	uint32_t *zutex;
	struct s_pointer_package {
		ZAS zas;
 		ZutexHandle *self;
	} pp;

	bool _zas_check(ZutexWaitSpec *spec);

	static bool _zas_check_s(ZoogDispatchTable_v1 *zdt, ZAS *zas, ZutexWaitSpec *spec);
	static ZASMethods3 _zutex_handle_zas_method_table;
};
