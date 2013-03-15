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
#include "pal_abi/pal_abi.h"

#ifdef __cplusplus
extern "C" {
#endif

#define xax_assert(x)	{ if (!(x)) { int *__y=0; *__y=7; while (1); } }
	// NB while(1) after segfault is there to make the compiler treat
	// this section as noreturn

extern void xint_msg(char *s);
#define XAX_CHEESY_DEBUG(s)	xint_msg(s)

	// let num_decimals = -1 for max decimals
void debug_logfile_append(ZoogDispatchTable_v1 *zdt, const char *logfile_name, const char *message);
uint32_t get_gs_base(void);
void xax_unimpl_assert(void);

void block_forever(ZoogDispatchTable_v1 *zdt);

#ifdef __cplusplus
}
#endif
