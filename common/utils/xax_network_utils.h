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
#include "pal_abi/pal_net.h"
#include "xax_network_defs.h"
#include "LiteLib.h"
#include "malloc_factory.h"
#include "zoog_network_order.h"

#ifdef __cplusplus
extern "C" {
#endif

bool decode_ipv4addr(const char *str, XIP4Addr *out_result);
XIPAddr decode_ipv4addr_assertsuccess(const char *str);
XIP4Addr literal_ipv4addr(uint8_t a, uint8_t b, uint8_t c, uint8_t d);
const char *decode_ipv6addr(const char *str, XIPAddr *out_addr);
XIPAddr decode_ipv6addr_assertsuccess(const char *str);

#define CONSUME_IPV4ADDR(a, f)	CONSUME_ARG(a, f, decode_ipv4addr_assertsuccess(value))

XIPAddr get_ip_subnet_broadcast_address(XIPVer version);
bool is_ip_subnet_broadcast_address(XIPAddr *addr);

typedef struct {
// outputs:
	XIPAddr source;
	XIPAddr dest;
	uint8_t layer_4_protocol;
	uint32_t payload_offset;
	uint32_t payload_length;
	uint32_t packet_length;
		// TODO not quite right, as a jumbogram + header can exceed uint32_t
	bool more_fragments;
	uint16_t fragment_offset;
	uint16_t identification;

// private (internal intermediate state)
	const char *failure;
	bool packet_valid;
	bool found_jumbo_length;
} IPInfo;

void decode_ip_packet(IPInfo *info, void *packet, uint32_t capacity);

// asserting accessors
XIP4Addr get_ip4(XIPAddr *addr);
XIP6Addr get_ip6(XIPAddr *addr);
XIPAddr ip4_to_xip(XIP4Addr *addr);
XIPAddr literal_ipv4xip(uint8_t a, uint8_t b, uint8_t c, uint8_t d);

uint32_t hash_xip(XIPAddr *addr);
int cmp_xip(XIPAddr *a, XIPAddr *b);

UDPEndpoint *make_UDPEndpoint(MallocFactory *mf, XIPAddr *addr, uint16_t port);

XIPAddr xip_all_ones(XIPVer ver);
XIPAddr xip_all_zeros(XIPVer ver);
XIPAddr xip_broadcast(XIPAddr *subnet, XIPAddr *netmask);

int XIPVer_to_idx(XIPVer ver);
XIPVer idx_to_XIPVer(int i);

#ifdef __cplusplus
}
#endif
