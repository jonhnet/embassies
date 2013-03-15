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

#include "pal_abi/pal_abi.h"	// ZoogDispatchTable_v1 type

#ifdef __cplusplus
extern "C" {
#endif

// this "class" is used by some C-only sources still. Bleeeccch.
struct s_pal_state;
typedef struct s_pal_state PalState;

PalState *pal_state_init(void);

int _gsfree_gettid(void);

ZoogDispatchTable_v1 *xil_get_dispatch_table();
void _xil_connect(ZPubKey *pub_key);
void _xil_start_receive_net_buffer_alarm_thread();

void *debug_alloc_protected_memory(size_t length);
void debug_announce_thread(void);

int _xil_zutex_wake_inner(
	uint32_t *zutex,
	uint32_t match_val, 
	Zutex_count n_wake,
	Zutex_count n_requeue,
	uint32_t *requeue_zutex,
	uint32_t requeue_match_val);

void xil_alarm_poll(uint32_t *known_sequence_numbers, uint32_t *new_sequence_numbers);

#ifdef __cplusplus
}
#endif
