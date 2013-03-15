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

#ifdef __cplusplus
extern "C" {
#endif

#include "pal_abi/pal_abi.h"
#include "malloc_factory.h"
#include "linux_syscall_ifc.h"
#include "cheesylock.h"

typedef struct {
	int number;
	char *name;
	char *argtypes;
} SyscallNameEntry;

typedef void (strace_time_ifc_f)(void *v_this, int* out_real_ms, int* out_user_ms);


typedef struct {
	SyscallNameEntry *syscall_entries;
	SyscallNameEntry *socketcall_entries;
	ZoogDispatchTable_v1 *xdt;
	strace_time_ifc_f *time_ifc;
	void *time_obj;
	bool time_initted;
	int real_time_base;
	int user_time_base;
} Strace;

void strace_init(
	Strace *strace,
	MallocFactory *mf,
	ZoogDispatchTable_v1 *xdt,
	strace_time_ifc_f *time_ifc,
	void *time_obj);
void strace_emit_trace(Strace *strace, UserRegs *ur, uint32_t return_val);

#ifdef __cplusplus
}
#endif
