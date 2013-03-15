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

#include "ElfArgs.h"

#ifdef __cplusplus
extern "C" {
#endif

void xinterpose_init(ZoogDispatchTable_v1 *xdt, uint32_t stack_base, StartupContextFactory *scf);

typedef void (xinterpose_f)(void);

void xinterpose(void);

#define RESTORE_USER_REGS(ur) \
	__asm__(	\
		"movl	%0,%%eax;"	\
		"movl	%1,%%ebx;"	\
		"movl	%2,%%ecx;"	\
		"movl	%3,%%edx;"	\
		"movl	%4,%%esi;"	\
		"movl	%5,%%edi;"	\
		"movl	%6,%%ebp;"	\
		: /*no output registers*/	\
		: "m"(ur.eax),	\
		  "m"(ur.ebx),	\
		  "m"(ur.ecx),	\
		  "m"(ur.edx),	\
		  "m"(ur.esi),	\
		  "m"(ur.edi),	\
		  "m"(ur.ebp)	\
		: "%eax",	\
		  /*"%ebx", Okay, fine I won't tell you about it. */	\
		  "%ecx",	\
		  "%edx",	\
		  "%esi",	\
		  "%edi"	\
		);
	/* astonishingly, the "no registers" comments must be present
	to avoid a syntax error! Compiler stupidity. */

#ifdef __cplusplus
}
#endif
