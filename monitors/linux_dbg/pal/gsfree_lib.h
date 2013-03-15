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

#define gsfree_assert(cond)	{ if (!(cond)) { __gsfree_assert(__FILE__, __LINE__, #cond); }}

void __gsfree_perf(const char* msg);
void __gsfree_assert(const char *file, int line, const char *cond);
void _gsfree_itoa(char *buf, int val);
void safewrite_str(const char *str);
void safewrite_hex(uint32_t val);
void safewrite_dec(uint32_t val);

#ifdef __cplusplus
}
#endif
