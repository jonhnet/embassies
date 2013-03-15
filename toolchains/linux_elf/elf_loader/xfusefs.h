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

#error this interface rotted when we upgraded from C to C++ handle ifc

#ifdef __cplusplus
extern "C" {
#endif

#include "zqueue.h"

typedef struct {
	XaxFileSystem xfs;
	ZQueue *zqueue;
	MallocFactory *mf;
	ZoogDispatchTable_v1 *xdt;
} XFuseFS;

void xfusefs_init(XFuseFS *xfusefs, MallocFactory *mf, ZoogDispatchTable_v1 *xdt, consume_method_f consume_method);

#ifdef __cplusplus
}
#endif
