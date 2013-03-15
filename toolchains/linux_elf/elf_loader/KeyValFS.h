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

#include "pal_abi/pal_abi.h"
#include "XaxVFS.h"
#include "xax_posix_emulation.h"
#include "KeyVal.h"
#include "SocketFactory_Skinny.h"
#include "xax_network_defs.h"
#include "KeyValClient.h"

class KeyValFS;

class KeyValFSHandle : public XaxVFSHandlePrototype
{
public:
	KeyValFSHandle(KeyValClient* kv_client, Key* key, PerfMeasure* perf);

	void ftruncate(
				XfsErr *err, int64_t length);
  virtual void read(
    XfsErr *err, void *dst, size_t len, uint64_t offset);
  virtual void write(
    XfsErr *err, const void *src, size_t len, uint64_t offset);

  virtual void xvfs_fstat64(XfsErr *err, struct stat64 *buf);

	virtual bool is_stream();
	virtual uint64_t get_file_len();

private:
	KeyValClient* kv_client;
	Key* key;	// Key representing the mount-point (currently the file)
	PerfMeasure* perf_measure;
};

class KeyValFS : public XaxVFSPrototype
{
public:
	KeyValFS(XaxPosixEmulation* xpe, const char* path, bool encrypted);
	XaxVFSHandleIfc *open(XfsErr *err, XfsPath *path, int oflag, XVOpenHooks *xoh=NULL);

	MallocFactory *get_mf() { return mf; }

private:
	MallocFactory *mf;
	KeyValClient* kv_client;
	PerfMeasure* perf_measure;
	Key* key;	// Key representing the mount-point (currently the file)
};

KeyValFS *xax_create_keyvalfs(XaxPosixEmulation *xpe, const char *path, bool encrypted);

