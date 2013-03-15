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

#define USE_FILE_IO 1
#include "ElfLoad.h"

#ifdef __cplusplus
extern "C" {
#endif


#define MAX_ENV_STR	12800

typedef struct {
	char string_space[MAX_ENV_STR];
	char *next_space;
} EnvBucket;

typedef enum {
	eb_arg,
	eb_env,
} EBType;

void eb_init(EnvBucket *eb);
void eb_add(EnvBucket *eb, EBType type, const char *assignment);
const char *envp_find(const char **envp, const char *name);
void envp_dump(char **envp);
void eb_dump(EnvBucket *eb);
void *get_xax_dispatch_table(const char **incoming_envp);
void launch_interpreter(ElfLoaded *elf_interp, const char *arg_env_str, void *xdt);


#ifdef __cplusplus
}
#endif
