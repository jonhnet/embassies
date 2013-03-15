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
#include "linked_list.h"
#include "XRamFS.h"

class XRDirEnt {
public:
	XRDirEnt(char *name, XRNode *xrnode);

public:
	char *name;
	XRNode *xrnode;
};

class XRDirNode : public XRNode {
public:
	XRDirNode(XRamFS *xramfs);

	XRNodeHandle *open_node(XfsErr *err, const char *path, int oflag, XVOpenHooks *xoh);
	void mkdir_node(XfsErr *err, const char *new_name, mode_t mode);
	void unlink_node(XfsErr *err, char *child);

	uint32_t get_dir_len(XfsErr *err);

	uint32_t getdent64(
			XfsErr *err, struct xi_kernel_dirent64 *dirp,
			uint32_t item_offset, uint32_t item_count);

private:
	LinkedList /*XRDirEnt*/ entries;

	XRNode *_create_per_hooks(int oflag, XVOpenHooks *xoh);
	void _insert_child(const char *name, XRNode *child);
};


class XRDirNodeHandle : public XRNodeHandle
{
public:
	XRDirNodeHandle(XRDirNode *dir_node);

	virtual bool is_stream() { return false; }
	virtual uint64_t get_file_len();
	void xvfs_fstat64(XfsErr *err, struct stat64 *buf);
	uint32_t get_dir_len(XfsErr *err);
	uint32_t getdent64(
			XfsErr *err, struct xi_kernel_dirent64 *dirp,
			uint32_t item_offset, uint32_t item_count);

private:
	XRDirNode *_get_dir_node() { return (XRDirNode*) get_xrnode(); }
};

