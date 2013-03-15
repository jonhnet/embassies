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

//#include <sys/types.h>	// size_t

typedef struct s_MallocFactory {
	void *(*c_malloc)(struct s_MallocFactory *mf, size_t size);
	void (*c_free)(struct s_MallocFactory *mf, void *ptr);
} MallocFactory;

static inline void *mf_malloc(MallocFactory *mf, size_t size)
	{ return (mf->c_malloc)(mf, size); } 

static inline void mf_free(MallocFactory *mf, void *ptr)
	{ (mf->c_free)(mf, ptr); } 

extern char *mf_strdup(MallocFactory *mf, const char *str);
extern char *mf_strndup(MallocFactory *mf, const char *str, int max_len);
extern void *mf_malloc_zeroed(MallocFactory *mf, size_t size);
extern void *mf_realloc(MallocFactory *mf, void *ptr, size_t old_size, size_t new_size);
	// NB our realloc makes caller track old size. The alternative is to
	// make each implementation do that, which they do anyway, huh? Well,
	// I'm lazy. Sorry.

#ifdef __cplusplus
}
#endif
