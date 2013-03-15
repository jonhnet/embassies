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
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct s_elf_reader_ifc;
typedef struct s_elf_reader_ifc ElfReaderIfc;
typedef bool (elf_read_f)(ElfReaderIfc *eri, uint8_t *buf, uint32_t offset, uint32_t length);
typedef void *(elf_read_alloc_f)(ElfReaderIfc *eri, uint32_t offset, uint32_t length);
	// Used by elfobj to read small sections.
	// ElfFlatReader has a simple, non-allocating implementation, which
	// is good for spartan environments like elf_loader.
	// The zftp reader can be fancy, allocating space as needed
	// and freeing later, so that elfobj doesn't have to think about
	// the fact that the file is loaded incrementally.

typedef void (elf_dtor_f)(ElfReaderIfc *eri);

struct s_elf_reader_ifc {
	elf_read_f *elf_read;
	elf_read_alloc_f *elf_read_alloc;
	elf_dtor_f *elf_dtor;
	uint32_t size;
};

#ifdef __cplusplus
}
#endif
