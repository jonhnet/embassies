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
// A half-baked discovery protocol, meant to be used by really stupid
// clients as they're bootstrapping. Likely want to replace with DHCP
// or other v4 or v6 discovery protocols as we mature.

#pragma once

#include "pal_abi/pal_types.h"
#include "pal_abi/pal_net.h"

#define discovery_port (7661)
#define discovery_magic ((7661<<16) | 7661)

typedef struct {
    uint32_t magic;
} DiscoveryPacket;

typedef struct {
    uint16_t datum_length;	// counting this field
    uint16_t request_type;
} DiscoveryDatum;

typedef enum {
    host_ifc_addr=0x10,
    host_ifc_app_range=0x11,
    router_ifc_addr=0x12,
    local_zftp_server_addr=0x13,
} DiscoveryType;

typedef struct {
    DiscoveryDatum hdr;
    XIP4Addr host_ifc_addr;
} Discovery_datum_host_ifc_addr;

typedef struct {
    DiscoveryDatum hdr;
    XIP4Addr host_ifc_app_range_low;
    XIP4Addr host_ifc_app_range_high;
} Discovery_datum_host_ifc_app_range;

typedef struct {
    DiscoveryDatum hdr;
    XIP4Addr router_ifc_addr;
} Discovery_datum_router_ifc_addr;

typedef struct {
    DiscoveryDatum hdr;
    XIP4Addr local_zftp_server_addr;
} Discovery_datum_local_zftp_server_addr;

