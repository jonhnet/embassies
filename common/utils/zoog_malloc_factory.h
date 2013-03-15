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

#include "concrete_mmap_xdt.h"
#include "cheesy_malloc_factory.h"
#include "malloc_factory.h"
#include "stomp_detector.h"
#include "abstract_mmap.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	ConcreteMmapXdt concrete_mmap_xdt_private;
	StompDetector stomp_detector;
	AbstractMmapIfc *cheesy_mmap_ifc;
	AbstractMmapIfc *guest_mmap_ifc;
	AbstractMmapIfc *brk_mmap_ifc;
	CheesyMallocArena cheesy_arena;
	CheesyMallocFactory cmf;
	MallocFactory *mf;
} ZoogMallocFactory;

void zoog_malloc_factory_init(ZoogMallocFactory *zmf, ZoogDispatchTable_v1 *xdt);
MallocFactory *zmf_get_mf(ZoogMallocFactory *zmf);

#ifdef __cplusplus
}
#endif
