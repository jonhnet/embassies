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

#include <stdint.h>
#include "malloc_factory.h"

// NB jonh abashadly recycling MTA object for runtime profiler, too,
// reusing fields for a different meaning here. Too tired to factor correctly
// right now with GI Joe Polymorphing Action.

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Track one allocation, recording its call stack.
// These are freed when the allocation is freed.
typedef struct {
	uint32_t addr;
	uint32_t size;
	uint32_t accum_size;
	uint32_t depth;
	uint32_t pc[0];
} MTAllocation;

MTAllocation *collect_fingerprint(MallocFactory *mf, int depth, int skip, void *start_ebp);

uint32_t fp_hashfn(const void *obj);
int fp_comparefn(const void *o0, const void *o1);

#ifdef __cplusplus
}
#endif // __cplusplus

