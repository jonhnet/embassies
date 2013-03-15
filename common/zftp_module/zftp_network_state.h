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
#ifndef _ZFTP_NETWORK_STATE_H
#define _ZFTP_NETWORK_STATE_H

#if 0 // dead code

#include "malloc_factory.h"
#include "xax_network_defs.h"
#include "zftp_protocol.h"

/****************************************************************************
 * ZFTPNetworkState abstracts over whether we're embedded in xax libc/ld.so
 * or free-standing on unix command line (test version).
 ****************************************************************************/

struct zftp_network_state;
typedef void (*zm_virtual_destructor_t)(struct zftp_network_state *netstate);
typedef void (*zm_get_endpoints_t)(struct zftp_network_state *netstate, UDPEndpoint *out_local_ep, UDPEndpoint *out_remote_ep);
typedef void (*zm_send_packet_t)(struct zftp_network_state *netstate, IPPacket *ipp);
typedef int (*zm_recv_packet_t)(struct zftp_network_state *netstate, uint8_t *buf, int data_len, UDPEndpoint *sender_ep);

typedef struct {
	zm_virtual_destructor_t zm_virtual_destructor;
	zm_get_endpoints_t zm_get_endpoints;
	zm_send_packet_t zm_send_packet;
	zm_recv_packet_t zm_recv_packet;
} ZFTPNetworkMethods;

typedef struct zftp_network_state {
	ZFTPNetworkMethods methods;
	MallocFactory *mf;
} ZFTPNetworkState;


void free_network_state(ZFTPNetworkState *netstate);

/****************************************************************************/

struct sockaddr;


// Used to roll metadata+root_hash into overall file_hash
typedef struct
{
	ZFTPFileMetadata metadata;
	hash_t root_hash;
} ZFTPFileHashContents;

#endif

#endif // _ZFTP_NETWORK_STATE_H
