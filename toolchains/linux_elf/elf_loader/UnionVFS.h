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
#include "linked_list.h"

class UnionVFS : public XaxVFSPrototype {
public:
	UnionVFS(MallocFactory *mf);
	~UnionVFS();

	void push_layer(XaxVFSIfc *vfs);

	XaxVFSIfc *pop_layer();

	virtual XaxVFSHandleIfc *open(
		XfsErr *err, XfsPath *path, int oflag, XVOpenHooks *open_hooks = NULL);

	virtual void mkdir(
		XfsErr *err, XfsPath *path, const char *new_name, mode_t mode);

	virtual void stat64(
		XfsErr *err,
		XfsPath *path, struct stat64 *buf);

protected:
	LinkedList layers;
};
