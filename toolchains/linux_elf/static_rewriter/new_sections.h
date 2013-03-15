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
#ifndef _NEW_SECTIONS_H
#define _NEW_SECTIONS_H

#include <stdint.h>
#include <elf.h>
#include <assert.h>

#include "elfobj.h"

#define MAX_SECTIONS 30

typedef struct s_new_section {
	int idx;
	uint32_t size;
	void *mapped_address;
	uint32_t file_offset;
	uint32_t vaddr;
	char *name;
	uint32_t extra_shstrtab_name_idx;
	Elf32_Word sh_type;
} NewSection;

typedef struct {
	uint32_t size;
	void *mapped_address;
	uint32_t file_offset;
	uint32_t vaddr;
	uint32_t next_allocation;
	NewSection newsections[MAX_SECTIONS];
	int num_new_sections;
	int next_idx;
	char *extra_shstrtab_space;
	int extra_shstrtab_size;
	int extra_shstrtab_offset;
} NewSections;


void nss_init(NewSections *nss, void *mapped_address, uint32_t vaddr, uint32_t file_offset, uint32_t size, int num_new_sections, int extra_shstrtab_size);
NewSection *nss_alloc_section(NewSections *nss, char *name, uint32_t size);
void nss_write_sections(NewSections *nss, NewSection *ns_shdrs, ElfObj *eo);

#endif // _NEW_SECTIONS_H
