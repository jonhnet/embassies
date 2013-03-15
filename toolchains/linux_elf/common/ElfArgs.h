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
#include "pal_abi/pal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	void *space;

	int capacity;
	int ptr_len;

	int argc;
	char *string_ptr;
	int string_len;

	const char **envp;	// for sc_lookup_env
} StartupContext;


typedef struct s_startup_context_factory {
	const char *configuration;
	void *xax_dispatch_table;
	StartupContext count_context;
	StartupContext fill_context;
} StartupContextFactory;

uint32_t scf_count(StartupContextFactory *scf, const char *configuration, void *xax_dispatch_table);
void scf_fill(StartupContextFactory *scf, void *space);

const char *scf_lookup_env(StartupContextFactory *scf, const char *key);
bool scf_lookup_env_bool(StartupContextFactory *scf, const char *key);

#ifdef __cplusplus
}
#endif
