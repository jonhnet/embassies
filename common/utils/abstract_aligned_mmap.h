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

struct s_abstract_aligned_mmap_ifc;
typedef struct s_abstract_aligned_mmap_ifc AbstractAlignedMmapIfc;

typedef void *(abstract_aligned_mmap_f)(AbstractAlignedMmapIfc *mmapifc, size_t length, uint32_t alignment);

typedef void (abstract_aligned_munmap_f)(AbstractAlignedMmapIfc *mmapifc, void *addr, size_t length);

struct s_abstract_aligned_mmap_ifc {
	abstract_aligned_mmap_f *mmap_f;
	abstract_aligned_munmap_f *munmap_f;
};
