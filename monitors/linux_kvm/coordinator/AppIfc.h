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

#include <pthread.h>
#include "malloc_factory.h"
#include "linked_list.h"
#include "NetIfc.h"
#include "Packet.h"

class App;

class AppIfc {
public:
	AppIfc(XIPAddr addr, App *app);
	AppIfc(XIPAddr addr);	// key constructor

	// avl tree methods
	AppIfc getKey() { return AppIfc(addr); }
	bool operator<(AppIfc b)
		{ return cmp(this, &b)<0; }
	bool operator>(AppIfc b)
		{ return cmp(this, &b)>0; }
	bool operator==(AppIfc b)
		{ return cmp(this, &b)==0; }
	bool operator<=(AppIfc b)
		{ return cmp(this, &b)<=0; }
	bool operator>=(AppIfc b)
		{ return cmp(this, &b)>=0; }


	App *get_app() { return app; }

	XIPVer get_version() { return addr.version; }
	XIPAddr *dbg_get_addr() { return &addr; }

	static uint32_t hash(const void *v_a);
	static int cmp(const void *v_a, const void *v_b);

private:
	XIPAddr addr;
	App *app;
};

