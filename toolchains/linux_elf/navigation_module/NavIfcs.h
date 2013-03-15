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

#include "NavigationProtocol.h"

class EntryPointIfc {
public:
	virtual ~EntryPointIfc() {}
	virtual void serialize(NavigationBlob* blob) = 0;
	virtual const char* repr() = 0;
};

class EntryPointFactoryIfc {
public:
	virtual EntryPointIfc* deserialize(NavigationBlob* blob) = 0;
};

class NavCalloutIfc {
public:
	virtual void transfer_forward(const char *vendor_name, EntryPointIfc* entry_point) = 0;
		// this copies vendor_name, keeps entry_point
	virtual void transfer_backward() = 0;
};

class NavigateIfc {
public:
	//virtual ~NavigateIfc() {}
	virtual void setup_callout_ifc(NavCalloutIfc *callout_ifc) = 0;
	virtual void navigate(EntryPointIfc *entry_point) = 0;
	virtual void no_longer_used() = 0;
	virtual void pane_revealed(ViewportID pane_tenant_id) = 0;
	virtual void pane_hidden() = 0;
};

class NavigateFactoryIfc {
public:
	virtual NavigateIfc *create_navigate_server() = 0;
		// return NULL if you can't navigate anymore, and we won't answer
		// any requests.
};

