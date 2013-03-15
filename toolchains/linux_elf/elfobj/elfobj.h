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

#include <unistd.h>
#include <elf.h>
#include <stdbool.h>
#include <malloc.h>

#include "elf_reader_ifc.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef STATIC_WORD_SIZE
#error Need -DSTATIC_WORD_SIZE=<n> compiler flag
#endif

#if STATIC_WORD_SIZE==32
#define ElfWS_Ehdr	Elf32_Ehdr
#define ElfWS_Phdr	Elf32_Phdr
#define ElfWS_Shdr	Elf32_Shdr
#define ElfWS_Sym	Elf32_Sym
#define ElfWS_Word	Elf32_Word
#define ElfWS_SWord	Elf32_SWord
#define ElfWS_Off	Elf32_Off
#define ElfWS_Dyn	Elf32_Dyn
#define ElfWS_Rel	Elf32_Rel
#define ELFWS_ST_TYPE	ELF32_ST_TYPE
#elif STATIC_WORD_SIZE==64
#define ElfWS_Ehdr	Elf64_Ehdr
#define ElfWS_Phdr	Elf64_Phdr
#define ElfWS_Shdr	Elf64_Shdr
#define ElfWS_Sym	Elf64_Sym
#define ElfWS_Word	Elf64_Word
#define ElfWS_SWord	Elf64_SWord
#define ElfWS_Off	Elf64_Off
#define ElfWS_Dyn	Elf64_Dyn
#define ElfWS_Rel	Elf64_Rel
#define ELFWS_ST_TYPE	ELF64_ST_TYPE
#else
#error Unknown STATIC_WORD_SIZE value
#endif

typedef struct {
	ElfReaderIfc *eri;

	ElfWS_Ehdr *ehdr;

	ElfWS_Shdr *shdr;
	ElfWS_Phdr *phdr;

	// section header string table
	ElfWS_Shdr *shstrtab_sec;
	const char *shstrtable;

	// main string table
	const char *strtable;

	ElfWS_Shdr *symtab_section;
	ElfWS_Sym *_symtab;
} ElfObj;

#if ELFOBJ_USE_LIBC
ElfWS_Phdr **elfobj_get_phdrs(ElfObj *eo, ElfWS_Word p_type, int *count);
#endif // ELFOBJ_USE_LIBC

bool elfobj_scan(ElfObj *eo, ElfReaderIfc *eri);
void elfobj_free(ElfObj *eo);

ElfWS_Phdr *elfobj_find_segment_by_type(ElfObj *eo, ElfWS_Word type);

ElfWS_Shdr *elfobj_find_shdr_by_type(ElfObj *eo, ElfWS_Word type);
const char *elfobj_get_string(ElfObj *eo, const char *strtable, ElfWS_Word sring_table_index);
int elfobj_find_section_index_by_name(ElfObj *eo, const char *name);
ElfWS_Shdr *elfobj_find_shdr_by_name(ElfObj *eo, const char *name, /* OUT OPTIONAL */ int *out_section_idx);
void *elfobj_read_section_by_name(ElfObj *eo, const char *name, /* OUT OPTIONAL */ int *out_section_idx);

ElfWS_Off elfobj_map_virtual_to_offset(ElfObj *eo, ElfWS_Word virt);

ElfWS_Sym *elfobj_find_symbol(ElfObj *eo, char *symname);
typedef void (scan_symbol_f)(ElfObj *eo, ElfWS_Sym *sym, void *arg);
void elfobj_scan_all_funcs(ElfObj *eo, scan_symbol_f *func, void *arg);

ElfWS_Sym *elfobj_fetch_symtab(ElfObj *eo);

#ifdef __cplusplus
}
#endif
