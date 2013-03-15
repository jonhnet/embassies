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

#include "core_regs.h"
#include "malloc_factory.h"
#include "linked_list.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct s_core_segment {
	uint32_t vaddr;
	void *bytes;
	int size;
} CoreSegment;

typedef struct {
	MallocFactory *mf;
	LinkedList threads;
	LinkedList segments;
	CoreNote_Zoog corenote_zoog;
} CoreFile;

void corefile_init(CoreFile *c, MallocFactory *mf);
void corefile_add_thread(CoreFile *c, Core_x86_registers *regs, int thread_id);
void corefile_add_segment(CoreFile *c, uint32_t vaddr, void *bytes, int size);
void corefile_set_bootblock_info(CoreFile *c, void *bootblock_addr, const char *bootblock_path);
	// this should be set to the address at which the PAL loaded the
	// zoog boot block (the first thing that's opaque to the PAL)

typedef bool (write_callback_func)(void *user_file_obj, void *ptr, int len);
typedef int (tell_callback_func)(void *user_file_obj);
void corefile_write_custom(CoreFile *c, write_callback_func *write_func, tell_callback_func *optional_tell_func, void *user_file_obj);

uint32_t corefile_compute_size(CoreFile *c);

#if USE_FILE_IO
void corefile_write(FILE *fp, CoreFile *c);
#endif // USE_FILE_IO

#ifdef __cplusplus
}
#endif
