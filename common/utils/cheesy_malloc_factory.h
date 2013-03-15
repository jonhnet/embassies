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

#include "malloc_factory.h"
#include "cheesymalloc.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct {
	MallocFactory mf;
	CheesyMallocArena *cheesy_arena;
} CheesyMallocFactory;

void *cmf_malloc(MallocFactory *mf, size_t size);
void cmf_free(MallocFactory *mf, void *ptr);
void cmf_init(CheesyMallocFactory *cmf, CheesyMallocArena *cheesy_arena);

#ifdef __cplusplus
}
#endif // __cplusplus
