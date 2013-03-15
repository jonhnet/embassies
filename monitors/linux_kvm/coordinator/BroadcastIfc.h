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
#include "AppLink.h"
#include "TunnelIfc.h"

class BroadcastIfc : public NetIfc
{
public:
	enum BroadcastVariant { ONES, LINK };
	BroadcastIfc(AppLink *app_link, TunnelIfc *tunnel_ifc, BroadcastVariant variant);
	const char *get_name();
	void deliver(Packet *p);
	XIPAddr get_subnet(XIPVer ver);
	XIPAddr get_netmask(XIPVer ver);

private:
	static void _deliver_one(void *v_packet, void *v_app_ifc);

	AppLink *app_link;
	TunnelIfc *tunnel_ifc;
	BroadcastVariant variant;
};
