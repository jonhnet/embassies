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

#include <elf.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	Elf32_Addr base;
	Elf32_Ehdr *ehdr;
	Elf32_Addr displace;	// needed for fixing up entry point
	Elf32_Addr all_begin;
	Elf32_Addr all_end;
	const char *dbg_fn;
} ElfLoaded;

ElfLoaded scan_elf(void *scan_addr);

#if USE_FILE_IO
ElfLoaded load_elf(const char *fn);
#endif // USE_FILE_IO

#ifdef __cplusplus
}
#endif
