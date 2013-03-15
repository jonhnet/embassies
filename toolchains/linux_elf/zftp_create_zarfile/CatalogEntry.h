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

#include "Emittable.h"
#include "zarfile.h"
#include "FileSystemView.h"
#include "PHdr.h"

class CatalogEntry /* : public Emittable */ {
private:
	char *url;
	bool present;
	FileSystemView *file_system_view;

	PHdr* phdr;	// an Emittable object that ends up in path index

public:
	enum { FLAGS_NONE = 0 };
	
	CatalogEntry(const char *url, bool present, uint32_t len, uint32_t flags, FileSystemView *file_system_view);
	~CatalogEntry();

	PHdr *take_phdr();
	const char *get_url() { return url; }
	ZFSReader *get_reader() { return file_system_view->open_url(url); }
		// ChunkEntry accessor

	static int cmp_by_url(const void *v_a, const void *v_b);
};
