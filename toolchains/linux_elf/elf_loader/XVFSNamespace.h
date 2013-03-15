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

class XVFSMount
{
public:
	XVFSMount(const char *prefix, XaxVFSIfc *xfs);

	const char *prefix;
	XaxVFSIfc *xfs;
};

class XVFSNamespace
{
public:
	XVFSNamespace(MallocFactory *mf);
	void install(XVFSMount *mount);
	XVFSMount *lookup(XfsPath *path);

private:
	LinkedList mounts;

	int _prefix_match(const char *long_str, const char *prefix);
};
