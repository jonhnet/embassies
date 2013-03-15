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

// An implementation of AbstractAlignedMmapIfc that accepts a
// (non-alignment-specified) AbstractMmapIfc, allocates more than
// is needed, adjusts the start pointer to be aligned, and
// leaves the slop lying around unused.
// (The slop is correctly returned at unmap time, though.)

#include <stdint.h>
#include "abstract_aligned_mmap.h"
#include "abstract_mmap.h"

typedef struct {
	void *backing_ptr;
	uint32_t backing_length;
} AlignatorBackingRecord;

typedef struct {
	AbstractAlignedMmapIfc ami;
	AbstractMmapIfc *backing;
} Alignator;

void alignator_init(Alignator *alignator, AbstractMmapIfc *backing);
