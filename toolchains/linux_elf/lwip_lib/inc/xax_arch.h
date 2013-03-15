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
#ifndef _XAX_ARCH_H
#define _XAX_ARCH_H

#include <pthread.h>
#include <stdbool.h>
#include "zutex_sync.h"
#include "xax_extensions.h"
#include "LegacyZClock.h"
#include "concrete_mmap_xdt.h"

typedef struct {
	ZoogDispatchTable_v1 *zdt;
	LegacyZClock *zclock;
	ConcreteMmapXdt cmx;
	CheesyMallocArena arena;
} XaxMachinery;

struct sys_sem {
	XaxMachinery *xm;
	unsigned int c;
	ZCond zcond;
	ZMutex zmutex;
	bool verbose;
};

void xax_arch_sem_configure(ZoogDispatchTable_v1 *zdt);
struct sys_sem *sys_sem_new_internal(XaxMachinery *xm, u8_t count);
void sys_sem_free_internal(struct sys_sem *sem);

#endif // _XAX_ARCH_H
