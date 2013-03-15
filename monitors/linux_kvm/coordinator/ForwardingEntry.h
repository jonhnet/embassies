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
#include "pal_abi/pal_net.h"

class ForwardingEntry {
public:
	ForwardingEntry(NetIfc *net_ifc, XIPAddr addr, XIPAddr netmask, const char *name);
	bool mask_match(XIPAddr *test_addr);
	bool equals(ForwardingEntry *other);

	NetIfc *get_net_ifc() { return net_ifc; }
	void print(char *buf, int bufsiz);

private:
	static bool _buf_mask(void *vb0, void *vb1, void *vmask, int size);

	NetIfc *net_ifc;
	XIPAddr addr;
	XIPAddr netmask;
	const char *name;
};
