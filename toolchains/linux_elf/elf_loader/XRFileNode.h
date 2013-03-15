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
#include "XRamFS.h"
#include "linked_list.h"

class XRFileNode : public XRNode
{
public:
	XRFileNode(XRamFS *xrfs);
	XRFileNode(MallocFactory *mf);
		// disembodied ctor; object to be installed later. YUCK YUCK YUCK.

	XRNodeHandle *open_node(XfsErr *err, const char *path, int oflag, XVOpenHooks *xoh);
	void mkdir_node(XfsErr *err, const char *new_name, mode_t mode);
	
	XRNode *hard_link();

private:
	friend class XRFileNodeHandle;
	uint8_t *contents;
	uint32_t capacity;
	uint32_t size;
	MallocFactory *mf;

	void _init(MallocFactory *mf);
	void _expand(uint32_t min_new_capacity);
};

class XRFileNodeHandle : public XRNodeHandle
{
public:
	XRFileNodeHandle(XRFileNode *xrfnode);

	bool is_stream() { return false; }
	uint64_t get_file_len();

	void read(XfsErr *err, void *dst, size_t len, uint64_t offset);
	void write(XfsErr *err, const void *src, size_t len, uint64_t offset);
	void xvfs_fstat64(XfsErr *err, struct stat64 *buf);
	void ftruncate(XfsErr *err, int64_t length);

protected:
	XRFileNode *get_file_node() { return (XRFileNode *) get_xrnode(); }
};

