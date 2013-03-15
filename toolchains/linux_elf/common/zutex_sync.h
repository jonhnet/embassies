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

/*
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
*/
#include "pal_abi/pal_abi.h"
#include "cheesymalloc.h"

#ifdef __cplusplus
extern "C" {
#endif


//////////////////////////////////////////////////////////////////////////////
//
// Synchronization mechanisms built out of zutexes.
//
// We modify pthreads to use zutexes as their underlying interface
// to thread scheduling services, and pthreads provides a rich variety
// of tested, non-half-baked synchronization mechanisms.
//
// However, we need sync primitives to coordinate between threads down
// at the ld.so and lwip levels (to implement select/poll), where attaching
// pthreads would be ... difficult ... if at all possible.
// Thus these poorly-tested, half-baked synchronization mechanisms.
//
//////////////////////////////////////////////////////////////////////////////

void zutex_simple_wake(ZoogDispatchTable_v1 *xdt, uint32_t *zutex);

//////////////////////////////////////////////////////////////////////////////

typedef struct {
	uint32_t val;
} ZMutex;

void zmutex_init(ZMutex *zmutex);
void zmutex_free(ZMutex *zmutex);
void zmutex_lock(ZoogDispatchTable_v1 *xdt, ZMutex *zmutex);
void zmutex_unlock(ZoogDispatchTable_v1 *xdt, ZMutex *zmutex);

//////////////////////////////////////////////////////////////////////////////

struct _s_zas;

typedef void (*zas_virtual_destructor_t)(ZoogDispatchTable_v1 *xdt, struct _s_zas *zas);
typedef bool (*zas_check_t)(ZoogDispatchTable_v1 *xdt, struct _s_zas *zas, ZutexWaitSpec *out_spec);
	// Returns true if the synchronization object won't block.
	// If it returns false, it populates *spec to prepare for a
	// kernel-scheduler wait-any.
typedef void (*zas_prewait_t)(ZoogDispatchTable_v1 *xdt, struct _s_zas *zas);
typedef void (*zas_postwait_t)(ZoogDispatchTable_v1 *xdt, struct _s_zas *zas);

typedef struct {
	zas_virtual_destructor_t virtual_destructor;	// NULL ok
	zas_check_t check;
	zas_prewait_t prewait;		// NULL ok
	zas_postwait_t postwait;	// NULL ok
} ZASMethods3;

typedef struct _s_zas {
	ZASMethods3 *method_table;
} ZAS;

int zas_check(ZoogDispatchTable_v1 *xdt, ZAS **zasses, int count);
	// returns -1 if none of the ZASses are ready.
	// TODO unused remove? Right now, for "cleanness", select with no
	// timeout depends on order-of-check propery of zas_wait_any,
	// falling through to a ZAlwaysReady to never wait.
int zas_wait_any(ZoogDispatchTable_v1 *xdt, ZAS **zasses, int count);

//////////////////////////////////////////////////////////////////////////////

// A dumb condition variable: it can only count to 1
typedef struct {
	ZAS zas;
	ZMutex mutex;
	uint32_t seq_zutex;
	bool auto_reset;
	bool signal;
} ZEvent;

void zevent_init(ZEvent *zevent, bool auto_reset);
void zevent_free(ZEvent *zevent);
void zevent_reset(ZoogDispatchTable_v1 *xdt, ZEvent *zevent);
void zevent_signal(ZoogDispatchTable_v1 *xdt, ZEvent *zevent, bool *dbg_oldstate);
bool zevent_is_set(ZoogDispatchTable_v1 *xdt, ZEvent *zevent);

//////////////////////////////////////////////////////////////////////////////

typedef struct _zpc_half {
	ZAS zas;
	struct _zpc_half *partner;
	ZEvent event;
	char dbg_desc[20];
} ZPCHalf;

void zpc_signal_half(ZoogDispatchTable_v1 *xdt, ZPCHalf *half);

typedef struct {
	ZPCHalf consumer_ready_half;	// consumer waits here, producer signals here
	ZPCHalf producer_idle_half;	// producer waits here, consumer signals here
} ZProducerConsumer;

void zpc_init(ZProducerConsumer *zpc, char *debug_string);
void zpc_free(ZProducerConsumer *zpc);
ZAS *zpc_get_consumer_zas(ZProducerConsumer *zpc);
ZAS *zpc_get_producer_zas(ZProducerConsumer *zpc);

//////////////////////////////////////////////////////////////////////////////

typedef struct {
	ZAS zas;
} ZAlwaysReady;

void zar_init(ZAlwaysReady *zar);
void zar_free(ZAlwaysReady *zar);

//////////////////////////////////////////////////////////////////////////////

typedef struct {
	ZMutex mutex;
	uint32_t seq_zutex;
} ZCond;

void zcond_init(ZCond *zcond);
void zcond_free(ZCond *zcond);
void zcond_signal(ZoogDispatchTable_v1 *xdt, ZCond *zcond);
void zcond_broadcast(ZoogDispatchTable_v1 *xdt, ZCond *zcond);
void zcond_wait(ZoogDispatchTable_v1 *xdt, ZCond *zcond, ZMutex *zmutex);

// To wait ZASsily on a ZCond, we need a per-thread structure. So
// construct one of these for each wait call, and use its ZAS.
// (Note that ZCond itself doesn't have a ZAS, because it's not
// ZASably waitable.)
typedef struct {
	ZAS zas;
	ZCond *zcond;
	ZMutex *zmutex;	// the associated mutex to release during wait
	uint32_t seq_value;
} ZCondWaitRecord;

ZCondWaitRecord *zcondwaitrecord_init_cheesy(CheesyMallocArena *arena, ZoogDispatchTable_v1 *xdt, ZCond *zcond, ZMutex *zmutex);
	// allocate with cheesymalloc
void zcondwaitrecord_init_auto(ZCondWaitRecord *zcwr, ZoogDispatchTable_v1 *xdt, ZCond *zcond, ZMutex *zmutex);
	// caller responsible for deallocation.
void zcondwaitrecord_free(ZoogDispatchTable_v1 *xdt, ZCondWaitRecord *zcwr);

#ifdef __cplusplus
}
#endif
