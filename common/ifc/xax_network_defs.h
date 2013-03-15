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
#ifndef _XAX_NETWORK_DEFS_H
#define _XAX_NETWORK_DEFS_H

#include "pal_abi/pal_types.h"
#include "pal_abi/pal_net.h"
//#include <stdint.h>

typedef struct s_ip4packet {
    uint8_t  version_and_ihl;
    uint8_t  type_of_service;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_and_fragment_offset;
#define MORE_FRAGMENTS_BIT			(0x2000)
#define FRAGMENT_OFFSET_MASK		(0x1fff)
#define FRAGMENT_GRANULARITY_BITS	(3)
    uint8_t  time_to_live;
    uint8_t  protocol;
    uint16_t header_checksum;
    XIP4Addr source_ip;
    XIP4Addr dest_ip;
} IP4Header;

typedef struct s_ip6header {
    uint32_t vers_class_flow;
    uint16_t payload_len;
    uint8_t next_header_proto;
    uint8_t hop_limit;
    XIP6Addr source_ip6;
    XIP6Addr dest_ip6;
} IP6Header;

typedef struct {
    uint8_t option_type;
    uint8_t option_length;
} XIP6Option;

typedef struct {
    uint8_t next_header_proto;
    uint8_t header_len;
} XIP6ExtensionHeader;

typedef struct {
    uint32_t jumbo_length;
} XIP6OptionJumbogram;

#define PROTOCOL_DEFAULT (0xff)
#define IP_PROTO_OPTION_HEADER	(0)
#define PROTOCOL_UDP (17)
#define UDP_PORT_FAKE_TIMER (13)
#define IP6_OPTION_TYPE_JUMBOGRAM	(0xc2)

typedef struct s_udppacket {
    uint16_t source_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} UDPPacket;

typedef struct s_udp_endpoint {
    XIPAddr ipaddr;
    uint16_t port;	// network order
} UDPEndpoint;

#define PORT_ANY	(0)

#define HSN_DATA_PORT (19820)

#endif // _XAX_NETWORK_DEFS_H
