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

#include "pal_abi/pal_abi.h"
#include "malloc_factory.h"

#ifdef __cplusplus
extern "C" {
#endif

// packetfilter.h now sits at the boundary between some old C code
// and some new C++
// code, and I don't want to have to keep recursing indefinitely
// far out. Eventually we'll get everything all good with C++ey types,
// but for now, I put up a void* to stem the bleeding.

struct s_XaxSkinnyNetworkPlacebo;
typedef struct s_XaxSkinnyNetworkPlacebo XaxSkinnyNetworkPlacebo;

typedef struct {
	ZoogDispatchTable_v1 *(*get_xdt)(XaxSkinnyNetworkPlacebo *xsnp);
	MallocFactory *(*get_malloc_factory)(XaxSkinnyNetworkPlacebo *xsnp);
} XaxSkinnyNetworkPlacebo_Methods;

struct s_XaxSkinnyNetworkPlacebo {
	XaxSkinnyNetworkPlacebo_Methods *methods;
};

#ifdef __cplusplus
}
#endif

