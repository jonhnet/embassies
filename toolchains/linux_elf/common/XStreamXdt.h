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

#include "XStream.h"
#include "pal_abi/pal_abi.h"
#include "pal_abi/pal_extensions.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct {
	XStream xstream;
	const char *dbg_stream_name;
	debug_logfile_append_f *dla;
} XStreamXdt;

void xsxdt_init(XStreamXdt *xsx, ZoogDispatchTable_v1 *xdt, const char *dbg_stream_name);

#ifdef __cplusplus
}
#endif // __cplusplus
