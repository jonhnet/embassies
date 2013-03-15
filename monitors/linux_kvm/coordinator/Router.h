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

#include "pal_abi/pal_net.h"
#include "linked_list.h"
#include "NetIfc.h"
#include "AppLink.h"
#include "TunnelIfc.h"
#include "TunIDAllocator.h"

class Router
{
public:
	Router(SyncFactory *sf, MallocFactory *mf, TunIDAllocator* tunid);

	void deliver_packet(Packet *p);

	void connect_app(App *app);
	void disconnect_app(App *app);

	NetIfc *lookup(XIPAddr *addr);
	XIPAddr get_gateway(XIPVer ver);
	XIPAddr get_netmask(XIPVer ver);

	void dbg_forwarding_table_print();

private:
	void _create_forwarding_table();
	void forwarding_table_add(NetIfc *ni);

	LinkedList forwarding_table;
	AppLink *app_link;
	TunnelIfc *tunnel_ifc;
};
