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

#include "LiteAssert.h"
#include "pal_abi/pal_basic_types.h"

// TODO x86 little-endian assumption.
// Implementing my own htonl & friends because I need them in ld.so.
static __inline uint64_t z_htonw(uint64_t input)
{
	return ((input&0x00000000000000ffLL)<<56)
		 | ((input&0x000000000000ff00LL)<<40)
		 | ((input&0x0000000000ff0000LL)<<24)
		 | ((input&0x00000000ff000000LL)<< 8)
		 | ((input&0x000000ff00000000LL)>> 8)
		 | ((input&0x0000ff0000000000LL)>>24)
		 | ((input&0x00ff000000000000LL)>>40)
		 | ((input&0xff00000000000000LL)>>56);
}
static __inline uint64_t z_ntohw(uint64_t input) { return z_htonw(input); }

static __inline uint32_t z_htonl(uint32_t input)
{
	return (input<<24)|((input&0xff00)<<8)|((input&0xff0000)>>8)|(input>>24);
}
static __inline uint32_t z_ntohl(uint32_t input) { return z_htonl(input); }

static __inline uint16_t z_htons(uint16_t input)
{
	return input<<8 | input>>8;
}
static __inline uint16_t z_ntohs(uint16_t input) { return z_htons(input); }

static __inline uint32_t z_ntohg(uint32_t input, uint32_t size)
{
	if (size==sizeof(uint32_t)) { return z_ntohl(input); }
	if (size==sizeof(uint16_t)) { return z_ntohs(input); }
	if (size==sizeof(uint8_t)) { return (input); }
	lite_assert(false);
	return -1;
}
static __inline uint32_t z_htong(uint32_t input, uint32_t size) { return z_ntohg(input, size); }

#define Z_NTOHG(v)	z_ntohg(v, sizeof(v))
// #define Z_HTONG(v)	z_htong(v, sizeof(v))
// Not supplied because you probably want to use sizeof the lvalue, not the input.


