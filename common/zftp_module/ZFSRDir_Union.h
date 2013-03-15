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

#include "ZFSRDir.h"
#include "avl2.h"
#include "SyncFactory.h"
#include "malloc_factory.h"

typedef AVLTree<HandyDirRec,HandyDirRec> HandyDirTree;

class ZFSRDir_Union : public ZFSRDir {
private:
	MallocFactory *mf;
	ZFSRDir *a;
	ZFSRDir *b;
	HandyDirTree tree;

protected:
	bool fetch_stat();
	bool scan();

private:
	void ingest(HandyDirRec *hdr);
	bool scan_one_dir(ZFSRDir *d);
	void squirt_tree();

public:
	ZFSRDir_Union(MallocFactory *mf, SyncFactory *sf, ZFSRDir *a, ZFSRDir *b);
};
