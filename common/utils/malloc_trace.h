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
#ifndef _MALLOC_TRACE_H
#define _MALLOC_TRACE_H

/*
 For docs, see monitors/linux_dbg/pal/memtracker.h
 */

#include <stdint.h>

#define MALLOC_TRACE_MALLOC 0
#define MALLOC_TRACE_MMAP 0
#define MALLOC_TRACE_CHEESY 0

typedef enum {
	tl_malloc = 0,
	tl_mmap = 1,
	tl_xil = 2,
	tl_cheesy = 3,
	tl_brk = 4,
	tl_COUNT = 8
} TraceLabel;

typedef struct {
	void (*tr_alloc)(TraceLabel label, void *addr, uint32_t size);
	void (*tr_free)(TraceLabel label, void *addr);
} MallocTraceFuncs;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
void xi_trace_alloc(MallocTraceFuncs *mtf, TraceLabel label, void *addr, uint32_t size);
void xi_trace_free(MallocTraceFuncs *mtf, TraceLabel label, void *addr);
#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _MALLOC_TRACE_H
