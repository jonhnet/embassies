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
#ifndef _ARGS_H
#define _ARGS_H

#include <stdbool.h>
#include <stdint.h>
#include "string_list.h"

typedef struct {
	char *input;
	char *output;
	bool dbg_symbols;
	uint32_t dbg_conquer_mask;
	uint32_t dbg_conquer_match;
	char *reloc_end_file;
	StringList ignore_funcs;
	bool keep_new_phdrs_vaddr_aligned;
} Args;

void args_init(Args *args, int argc, char **argv);

#endif // _ARGS_H
