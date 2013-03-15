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

#ifdef __cplusplus

#include "XaxVFS.h"
#include "ZCachedFile.h"
#include "ZFileClient.h"
#include "ZLookupClient.h"
#include "xax_posix_emulation.h"
#include "ZLCArgs.h"
#include "ZFTPDecoder.h"

class ZLCVFS;

class ZLCHandle : public ZFTPDecoder {
public:
	ZLCHandle(ZLCVFS *zlcvfs, ZCachedFile *zcf, char *url_for_stat);
	virtual ~ZLCHandle();

	virtual uint64_t get_file_len();
	virtual bool is_dir();

private:
	void _internal_read(void *buf, uint32_t count, uint32_t offset);

	ZLCVFS *zlcvfs;
	ZCachedFile *zcf;

};

class ZLCZCBHandle : public ZFTPDecoder {
public:
	ZLCZCBHandle(ZLCVFS *zlcvfs, ZeroCopyBuf *zcb);
	virtual ~ZLCZCBHandle();

	virtual uint64_t get_file_len();
	virtual bool is_dir();
	void* fast_mmap(size_t len, uint64_t offset);

private:
	void _internal_read(void *buf, uint32_t count, uint32_t offset);

	ZLCVFS *zlcvfs;
	ZeroCopyBuf *zcb;
};

class ZLCVFS : public XaxVFSPrototype {
public:
	ZLCVFS(XaxPosixEmulation* xpe, ZLCEmit* ze);
	virtual XaxVFSHandleIfc *open(
		XfsErr *err, XfsPath *path, int oflag, XVOpenHooks *open_hooks = NULL);

	static char *assemble_url(MallocFactory *mf, const char *scheme, const char *path);
	static void assemble_urls(MallocFactory *mf, XfsPath *path, char **out_url, char **out_url_for_stat);

private:
	friend class ZLCHandle;
	friend class ZLCZCBHandle;
	inline ZCache *get_zcache() { return zcache; }

	void _evil_get_server_addresses(
		UDPEndpoint *out_lookup_ep,
		UDPEndpoint *out_zftp_ep);
	static void start_file_client(void *v_zfc);
	bool _find_fast_fetch_origin(UDPEndpoint *out_origin);
	XaxVFSHandleIfc *_fast_fetch_zarfile(const char *fetch_url);
	

	XaxPosixEmulation *xpe;
	ZCache *zcache;
	ZLCArgs zlcargs;
	SocketFactory *socket_factory;
	ZLCEmit *ze;
	ZFileClient *zfile_client;
	ZLookupClient *zlookup_client;
};
#endif // __cplusplus
