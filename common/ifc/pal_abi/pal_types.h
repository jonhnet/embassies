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
/*----------------------------------------------------------------------
| PAL_TYPES
|
| Purpose: Process Abstraction Layer Type Definitions
|
| Remarks: We want to avoid dependency on system includes (for compiling
|   against self-contained subsystems), while remaining compatible
|   with them.
|   Thus, we define those boring system types needed by the pal_abi here.
----------------------------------------------------------------------*/

#pragma once

#include "pal_abi/pal_basic_types.h"

#define SIGNED_BINARY_MAGIC 0x5a53474e	/*'ZSGN'*/

typedef struct {
	uint32_t magic;			// ==SIGNED_BINARY_MAGIC, *network order*
	uint32_t cert_len;		// *network order*
	uint32_t binary_len;	// *network order*
	// followed by cert_len bytes of ZoogCert
	// followed by binary_len bytes of binary data following the zoog boot
	// block ABI.
} SignedBinary;

#ifndef __cplusplus
typedef void ZCert;
typedef void ZCertChain;
typedef void ZPubKey;
#else 
#include "ZPubKey.h"
#include "ZCert.h"
#include "ZCertChain.h"
#endif
