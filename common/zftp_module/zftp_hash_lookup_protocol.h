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

// An (insecure) protocol for converting <url_hint>s into <file_hash>s.
// We're using it while developing, because it's complicated/expensive
// to bake an application environment into a manifest, or to design/deploy
// a signed / channel-secured lookup protocol.

#include "zftp_protocol.h"
	// Reply packet uses ZFTPReplyCodes

#define ZFTP_HASH_LOOKUP_PORT	(7663)

#pragma pack(push)
#pragma pack(1)
typedef struct
{
	uint32_t nonce;
	uint16_t url_hint_len;

	// This struct followed by a variable-length region containing:
	// <url_hint> url_hint_len char string. Last byte must be 0.
} ZFTPHashLookupRequestPacket;

typedef struct
{
	uint32_t nonce;
	uint16_t hash_len;
	hash_t hash;
} ZFTPHashLookupReplyPacket;
#pragma pack(pop)

