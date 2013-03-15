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

#ifdef __cplusplus
extern "C" {
#endif

#include "elf_reader_ifc.h"


typedef struct {
	ElfReaderIfc ifc;
	uint8_t *bytes;
	bool this_owns_image;
} ElfFlatReader;

void elf_flat_reader_init(ElfFlatReader *efr, uint8_t *bytes, uint32_t size, bool this_owns_image);

#if ELFOBJ_USE_LIBC
void efr_read_file(ElfFlatReader *efr, const char *filename);
void efr_write_file(ElfFlatReader *efr, const char *filename);
#endif // ELFOBJ_USE_LIBC

#ifdef __cplusplus
}
#endif
