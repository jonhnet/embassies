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

#include "LiteLib.h"
#include "ZFTPDecoder.h"
#include "zarfile.h"
#include "ZLCEmit.h"
#include "SyncFactory.h"

class ZarfileVFS;
class ZarfileHandleChunkDecoder;
class ChdrHolder;

class ZarfileHandle : public ZFTPDecoder {
public:
	ZarfileHandle(ZarfileVFS *zvfs, ZF_Phdr *phdr, char *url_for_stat);
	~ZarfileHandle();
	virtual bool is_dir();
	virtual uint64_t get_file_len();

	virtual bool is_stream() { return false; }
	virtual uint32_t read(
		XfsErr *err, void *dst, size_t len) { lite_assert(false); return -1; }
	virtual void* fast_mmap(size_t len, uint64_t offset);

protected:
	virtual void _internal_read(void *buf, uint32_t count, uint32_t offset);

private:
	friend class ZarfileHandleChunkDecoder;
	ZarfileVFS *zvfs;
	ZF_Phdr *phdr;
};

class ZarfileVFS : public XaxVFSPrototype {
public:
	ZarfileVFS(MallocFactory *mf, SyncFactory* sf, XaxVFSHandleIfc *zarfile_hdl, ZLCEmit *ze);
	~ZarfileVFS();

	virtual XaxVFSHandleIfc *open(
		XfsErr *err, XfsPath *path, int oflag, XVOpenHooks *open_hooks = NULL);
		

	virtual void mkdir(
		XfsErr *err, XfsPath *path, const char *new_name, mode_t mode);

	virtual void stat64(
		XfsErr *err,
		XfsPath *path, struct stat64 *buf);

private:
	friend class ZarfileHandle;
	friend class ZarfileHandleChunkDecoder;
	friend class ChdrHolder;
	static int _phdr_key_cmp(const void *v_key, const void *v_member);
	ZF_Chdr* lock_chdr(uint32_t ci);
	void unlock_chdr(uint32_t ci, ZF_Chdr* chdr);

	MallocFactory *mf;
	XaxVFSHandleIfc *zarfile_hdl;
	ZLCEmit *ze;
	ZF_Zhdr zhdr;
	ZF_Phdr *ptable;
	char *strtable;
	ZF_Chdr *chdr;
	SyncFactoryMutex *chdr_mutex;
	int lost_mmap_opportunities_misaligned;
	int exploited_mmap_opportunities;
};
