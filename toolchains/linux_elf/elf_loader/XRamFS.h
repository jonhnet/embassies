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
#include "fake_ids.h"

class XRNode;

class XRNodeHandle : public XaxVFSHandlePrototype
{
public:
	XRNode *get_xrnode() { return xrnode; }

protected:
	XRNode *xrnode;

	XRNodeHandle(XRNode *xrnode);
};

class XRamFS;

class XRNode {
public:
	virtual XRNodeHandle *open_node(XfsErr *err, const char *path, int oflag, XVOpenHooks *xoh) = 0;
	virtual void mkdir_node(XfsErr *err, const char *new_name, mode_t mode) = 0;

	virtual XRNode *hard_link();
		// override to return a copy of this if you want to admit hard links.

	virtual void unlink_node(XfsErr *err, char *child);
		// override to return true if you can unlink child
	
	XRamFS *get_xramfs() { return xramfs; }

	void add_ref();
	void drop_ref();

protected:
	XRNode(XRamFS *xramfs);
	XRamFS *xramfs;
	int refcount;

protected:
	virtual ~XRNode() {}
};

class XRamFS : public XaxVFSIfc
{
public:
	XRamFS(MallocFactory *mf, FakeIds *fake_ids);
	XaxVFSHandleIfc *open(XfsErr *err, XfsPath *path, int oflag, XVOpenHooks *xoh=NULL);
	void mkdir(XfsErr *err, XfsPath *path, const char *new_name, mode_t mode);
	void stat64(XfsErr *err, XfsPath *path, struct stat64 *buf);
	void hard_link(XfsErr *err, XfsPath *src_path, XfsPath *dest_path);
	void unlink(XfsErr *err, XfsPath *path);

	FakeIds *get_fake_ids();
	void set_fake_ids(FakeIds *fake_ids);

	MallocFactory *get_mf() { return mf; }

	virtual XaxVirtualSocketProvider *as_socket_provider() { return NULL; }

private:
	XRNode *root;
	MallocFactory *mf;
	FakeIds *fake_ids;
};

XRamFS *xax_create_ramfs(XaxPosixEmulation *xpe, const char *path);

// ifcs from xprocfakery
#if 0
XRFileNode *xrfile_create(MallocFactory *mf);
XfsHandle *xrfile_open(XaxHandleTableIfc *xht, struct s_xrnode *xrnode, XfsErr *err, const char *path, int oflag, XaxOpenHooks *xoh);
void xrfilenode_write(
	XRFileNode *xrfile, int *i_offset, const void *buf, size_t count);
#endif
