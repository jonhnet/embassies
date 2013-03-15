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

#include <stdint.h>

#include "zftp_protocol.h"
#include "xax_network_defs.h"	// IPPacket
#include "malloc_factory.h"		// MallocFactory
#include "math_util.h"			// max, min

#ifdef __cplusplus
extern "C" {
#endif

struct s_ht_node;
#ifndef TYPE_HASHTREE_
#define TYPE_HASHTREE_
typedef struct s_ht_node HashTree;
#endif

char *zftp_hash2str(hash_t *hash, char *out_str);
void zftp_str2hash(const char *, hash_t *);

// IPPacket *zftp_build_ip_packet(MallocFactory *mf, uint8_t *buf, int data_len, UDPEndpoint local_ep, UDPEndpoint remote_ep);
	// used only by dead code (zftp_client_lib.c)

bool _is_all_zeros(uint8_t *in_data, uint32_t valid_data_len);

#ifdef __cplusplus
}
#endif
