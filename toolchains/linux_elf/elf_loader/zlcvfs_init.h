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

#include "ZarfileVFS.h"
#include "UnionVFS.h"
#include "SyncFactory_Zutex.h"

class ZLCVFS_Wrapper {
private:
	ZarfileVFS *zarfile_vfs;
	UnionVFS *union_vfs;
	XaxVFSIfc *vfs;
	SyncFactory_Zutex *sf;
	TraceCollector* traceCollector;

public:
	ZLCVFS_Wrapper(XaxPosixEmulation *xpe, const char *path, const char *zarfile_path, bool trace);
	void liberate();
};
