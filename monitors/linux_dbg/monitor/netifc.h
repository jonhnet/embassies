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

#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/socket.h>

#include "pal_abi/pal_abi.h"
#include "xax_network_defs.h"
#include "PacketQueue.h"
#include "hash_table.h"
#include "forwarding_table.h"
#include "TunIDAllocator.h"

class Monitor;

//#define USE_PROXY true
#define USE_PROXY false 
#define PROXY_IP "127.0.0.1"

struct s_net_connection;
typedef struct s_net_connection NetConnection;

typedef struct {
  XIPAddr netmask;
  XIPAddr subnet;	// subnet address (10.X.0.0)
  XIPAddr gateway;
} XIP_ifc_config;

typedef struct s_netifc {
	bool use_proxy;
	int proxy_sock;
	struct sockaddr_in proxy_addr;
	
	int tun_fd;
	pthread_t read_thread_obj;

	XIP_ifc_config ifc4;
	XIP_ifc_config ifc6;
	uint32_t app_index;

	ForwardingTable ft;

	HashTable netconn_ht;
	pthread_mutex_t netconn_ht_mutex;

	Monitor *monitor;

	PacketAccount *packet_account;
} NetIfc;

void net_ifc_init_privileged(NetIfc *ni, Monitor *monitor, TunIDAllocator* tunid);
void send_packet(NetIfc *ni, XPBPoolElt *xpbe);
void send_packet(NetIfc *ni, char *buf, uint32_t capacity);

struct s_app_interface;
typedef struct s_app_interface AppInterface;

struct s_net_connection {
	NetIfc *ni;
	XIPifconfig host_ifconfig;
	PacketQueue *pq;
	AppInterface* ai;
};

struct s_app_interface {
	NetConnection conn4;
	NetConnection conn6;
	PacketQueue pq;
};

void app_interface_init(AppInterface *ai, NetIfc *ni, EventSynchronizer *event_synchronizer);
void app_interface_free(AppInterface *ai, NetIfc *ni);

void app_interface_send_timer_tick(AppInterface *ai);
uint32_t net_connection_hash(const void *a);
int net_connection_cmp(const void *a, const void *b);
int net_connection_allocate_index(NetIfc *ni);
