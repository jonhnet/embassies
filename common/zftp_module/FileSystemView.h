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

#include "ZFSReader.h"
#include "malloc_factory.h"
#include "linked_list.h"
#include "SyncFactory.h"

class FileSystemView {
private:
	MallocFactory *mf;
	SyncFactory *sf;
	LinkedList overlays;
	uint32_t max_prefix;
	
	ZFSReader *_open_one_real_path(const char *path);

public:
	FileSystemView(MallocFactory *mf, SyncFactory *sf);
	~FileSystemView();
	void insert_overlay(const char *path);
		// puts path before all other prefixes in the overlay.
		// this copies path

	ZFSReader *open_path(const char *path);
	ZFSReader *open_url(const char *url_hint);
};
