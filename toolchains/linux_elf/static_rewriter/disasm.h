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
#ifndef _DISASSEMBLER_H
#define _DISASSEMBLER_H

#include <stdio.h>

#include "jump_target_list.h"

typedef struct {
	uint8_t *target_space_mapped_base;
	uint32_t target_space_vaddr_base;
	uint32_t target_capacity;
	uint32_t next_target_space_offset;
	uint32_t pogo_vaddr;
} Patcher;

typedef struct {
	Patcher *patcher;

	int num_funcs;
	int num_with_int80s;
} Disassembler;

uint32_t write_pogo(uint8_t *pogo_space_mapped_base);
void patcher_init(Patcher *patcher,
	uint8_t *target_space_mapped_base,
	uint32_t target_space_vaddr_base,
	uint32_t pogo_vaddr,
	uint32_t target_capacity);

void disasm_init(Disassembler *disasm, Patcher *patcher);
bool patch_func(Disassembler *disasm, unsigned char *start, unsigned int len, uint32_t virt_addr, JumpTargetList *external_jump_targets, const char *symname);


#endif // _DISASSEMBLER_H
