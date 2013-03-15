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

#include <pal_abi/pal_types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct s_cheesylock {
	uint32_t guard0;
	uint32_t value;
	uint32_t debug_frame_ptr;
	uint32_t guard1;
} CheesyLock;

void cheesy_lock_init(CheesyLock *cl);
void cheesy_lock_acquire(CheesyLock *cl);
void cheesy_lock_release(CheesyLock *cl);

#ifdef __cplusplus
}
#endif
