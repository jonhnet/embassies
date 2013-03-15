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
#ifndef _FUNCTABLE_H
#define _FUNCTABLE_H

#include "elfobj.h"
#include "jump_target_list.h"

struct s_funcentry;
typedef struct s_funcentry FuncEntry;

struct s_funcentry {
	Elf32_Sym sym;
	JumpTargetList jtl;
	FuncEntry *copied;
	int dbg_presort_index;
};

void func_entry_init(FuncEntry *fe, Elf32_Sym *sym, int dbg_presort_index);

typedef struct {
	ElfObj *eo;
	FuncEntry *table;
	int capacity;
	int count;
} FuncTable;

void func_table_init(FuncTable *ft, ElfObj *eo);
void func_table_free(FuncTable *ft);

#endif // _FUNCTABLE_H
