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

// Some libraries (crypto mpi) we're using want an ambient malloc.
// We used to avoid that at all costs (down in elf_loader layers) because
// of the pain of trying to find arenas globally, instead using an explicit
// MallocFactory object everywhere. But we finally fixed that assumption,
// so that now we have an ambient 'operator new'.
// Thus, we cave in and also provide ambient malloc, to avoid needing to
// thread MallocFactory objects through the crypto mpi lib.

#include "pal_abi/pal_basic_types.h"
#include "malloc_factory.h"

#ifdef __cplusplus
extern "C" {
#endif

void *ambient_malloc(size_t size);
void ambient_free(void *ptr);

void ambient_malloc_init(MallocFactory *mf);

#ifdef __cplusplus
}
#endif
