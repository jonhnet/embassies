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
#ifndef _CORE_REGS_H
#define _CORE_REGS_H

#undef __BEGIN_DECLS
#undef __END_DECLS
#ifdef __cplusplus
# define __BEGIN_DECLS extern "C" {
# define __END_DECLS }
#else
# define __BEGIN_DECLS /* empty */
# define __END_DECLS /* empty */
#endif

#include "pal_abi/pal_types.h"
//#include <stdint.h>
typedef int int32_t;
typedef long long int int64_t;
#include "local_elf.h"

typedef struct {
	uint32_t bx;
	uint32_t cx;
	uint32_t dx;
	uint32_t si;
	uint32_t di;
	uint32_t bp;
	uint32_t ax;
	uint32_t ds;
	uint32_t es;
	uint32_t fs;
	uint32_t gs;
	uint32_t orig_ax;
	uint32_t ip;
	uint32_t cs;
	uint32_t flags;
	uint32_t sp;
	uint32_t ss;
} Core_x86_registers;

typedef struct {
	uint8_t unknown0[0x18];
	uint32_t pid;
	uint32_t ppid;
	uint32_t pgrp;
	uint32_t sid;
	uint32_t unknown1[8];
	Core_x86_registers regs;
	uint32_t unknown2[1];
} CoreNote_desc;

typedef struct {
	Elf32_Nhdr nhdr;
	char name[8];
} NoteHeader;

typedef struct {
	NoteHeader note;
	CoreNote_desc desc;
} CoreNote_Regs;

typedef struct {
	NoteHeader note;
	uint32_t bootblock_addr;
	char bootblock_path[256];
} CoreNote_Zoog;

#endif // _CORE_REGS_H
