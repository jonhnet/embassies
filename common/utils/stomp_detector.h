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

#include <sys/types.h>
#include "abstract_mmap.h"
#include "cheesymalloc.h"
#include "cheesy_malloc_factory.h"
#include "ordered_collection.h"

typedef struct {
	// where *we* get storage (next turtle down)
	AbstractMmapIfc *raw_storage;
	// an allocator for allocating accounting structures
	CheesyMallocArena cma;
	CheesyMallocFactory cmf;
	MallocFactory *mf;

	// what we're storing
	OrderedCollection oc;
} StompDetector;

void stomp_init(StompDetector *sd, AbstractMmapIfc *raw_storage);

typedef struct {
	AbstractMmapIfc abstract_mmap_ifc;
	StompDetector *sd;
} StompRegion;

StompRegion *stomp_new_region(StompDetector *sd);
