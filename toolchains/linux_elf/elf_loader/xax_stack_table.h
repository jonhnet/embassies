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
#include "hash_table.h"
#include "zutex_sync.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct s_xax_stack_table_entry {
	uint32_t gs_thread_handle;	// entry key
//	void *tcb;	// thread control block address
	void *stack_base;
	int tid;
} XaxStackTableEntry;

typedef struct s_xax_stack_table {
	MallocFactory *mf;
	ZoogDispatchTable_v1 *xdt;
	ZMutex mutex;
	HashTable ht;
} XaxStackTable;

void xst_init(XaxStackTable *xst, MallocFactory *mf, ZoogDispatchTable_v1 *xdt);
void xst_insert(XaxStackTable *xst, XaxStackTableEntry *xte);
	// caller owns xte (this makes a copy)
XaxStackTableEntry *xst_peek(XaxStackTable *xst, uint32_t gs_thread_handle_key);
void xst_remove(XaxStackTable *xst, uint32_t gs_thread_handle_key, XaxStackTableEntry *out_xte);
	// results copied into out_xte

#ifdef __cplusplus
}
#endif
