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
#include "xax_network_utils.h"
#include "ZLCEmit.h"
#include "cheesy_snprintf.h"

#define ZLC_BASE(ze, level, f, ...)	{ \
	if (ze!=NULL && level>=ze->threshhold()) { \
		char _z_buf[1000]; \
		cheesy_snprintf(_z_buf, sizeof(_z_buf), f __VA_ARGS__); \
		ze->emit(_z_buf); \
	} \
  }

#define ZLC_TERSE(ze, f, ...)		{ ZLC_BASE(ze, terse, f, __VA_ARGS__); }
#define ZLC_CHATTY(ze, f, ...)		{ ZLC_BASE(ze, chatty, f, __VA_ARGS__); }
#define ZLC_COMPLAIN(ze, f, ...)	{ ZLC_BASE(ze, complaint, f, __VA_ARGS__); }

#define ZLC_VERIFY(ze, expr)		{ if (!(expr)) { ZLC_COMPLAIN(ze, "!%s\n",, #expr); goto fail; } }
