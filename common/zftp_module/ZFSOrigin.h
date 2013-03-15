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

#include "zftp_protocol.h"

#include "ZCache.h"
#include "ZOrigin.h"
#include "FreshnessSeal.h"
#include "FileSystemView.h"

class ZFSOrigin : public ZOrigin {
public:
	ZFSOrigin(ZCache *zcache);

	bool lookup_new(const char *url);
		// This url isn't cached in the ZNameDB.
		// Fetch it from the origin,
		// store its name_hash, file_hash and block hashes, and
		// return a ZNameRecord that provides file_hash.
		// NULL => no file found.

	bool validate(const char *url, FreshnessSeal *seal);

	const char *get_origin_name() { return "ZFSOrigin"; }

	// recycled by ZStatOrigin.
	static FreshnessSeal _all_links_mtime(const char *data_path);

private:
	const char *_url_to_path(const char *url);

	ZCache *zcache;
	FileSystemView file_system_view;
};
