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

class XerrHandle : public XaxVFSHandlePrototype
{
public:
	XerrHandle(ZoogDispatchTable_v1 *xdt, const char *prefix);

	virtual bool is_stream() { return true; }
	virtual uint64_t get_file_len() { lite_assert(false); return 0; }

	virtual uint32_t read(
		XfsErr *err, void *dst, size_t len);
	virtual void write(
		XfsErr *err, const void *src, size_t len, uint64_t offset);
	void xvfs_fstat64(
		XfsErr *err, struct stat64 *buf);
	

private:
	const char *prefix;
	ZoogDispatchTable_v1 *xdt;
};
