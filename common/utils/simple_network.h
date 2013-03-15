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

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "pal_abi/pal_types.h"
#include "pal_abi/pal_net.h"
#include "pal_abi/pal_abi.h"	// ZoogDispatchTable_v1
#include "xax_network_defs.h"

uint16_t ip_checksum(IP4Header *ipp);

ZoogNetBuffer *create_udp_packet_xnb(
	ZoogDispatchTable_v1 *zdt,
	UDPEndpoint source_ep, UDPEndpoint dest_ep, uint32_t payload_size,
	void **out_payload);
	// returns pointer to payload portion 
	// decides ipv4/ipv6 based on dest_ep.ipaddr.version

uint32_t udp_packet_length(XIPVer version, uint32_t payload_size);

void *create_udp_packet_buf(
	uint8_t *local_buffer, uint32_t buf_capacity, UDPEndpoint source_ep, UDPEndpoint dest_ep, uint32_t payload_size);

UDPEndpoint const_udp_endpoint_ip4(
	uint8_t oct0, uint8_t oct1, uint8_t oct2, uint8_t oct3, uint16_t port);

#ifdef __cplusplus
}
#endif // __cplusplus
