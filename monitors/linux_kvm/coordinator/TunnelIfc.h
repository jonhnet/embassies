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

#include "NetIfc.h"
#include "TunIDAllocator.h"

class Router;
class AppLink;

class TunnelIfc : public NetIfc
{
public:
	TunnelIfc(Router *router, AppLink *app_link, TunIDAllocator* tunid);
	void deliver(Packet *p);
	const char *get_name() { return "tunnel"; }
	XIPAddr get_subnet(XIPVer ver);
	XIPAddr get_netmask(XIPVer ver);

private:
	static void *_recv_thread(void *v_this);
	void _recv_thread();

	int tun_fd;
	pthread_t recv_thread;
	Router *router;
};
