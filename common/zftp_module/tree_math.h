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


#include "lite_log2.h"

#ifdef __cplusplus
extern "C" {
#endif
	
uint32_t _tree_index(int level, int col, uint32_t tree_size);
void _tree_unindex(uint32_t location, int *level_out, int *col_out);
uint32_t _left_child(uint32_t location);
uint32_t _right_child(uint32_t location);
uint32_t _sibling_location(uint32_t location);
uint32_t _parent_location(uint32_t location);

const hash_t *hash_of_block_of_zeros();

#ifdef __cplusplus
}
#endif
