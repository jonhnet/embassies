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
#ifndef _LINUX_SYSCALL_IFC_H
#define _LINUX_SYSCALL_IFC_H

#include <stdint.h>

typedef struct
{
	union {
		struct {
			uint32_t eax;	// syscall number
			uint32_t ebx;	// arg 1...
			uint32_t ecx;
			uint32_t edx;
			uint32_t esi;
			uint32_t edi;
			uint32_t ebp;	// ...arg 6
			uint32_t eip;
		};
		struct {
			uint32_t syscall_number;
			uint32_t arg1;
			uint32_t arg2;
			uint32_t arg3;
			uint32_t arg4;
			uint32_t arg5;
			uint32_t arg6;
		};
	};
	
} UserRegs;

#endif // _LINUX_SYSCALL_IFC_H
