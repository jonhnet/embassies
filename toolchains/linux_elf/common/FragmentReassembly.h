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

#include "pal_abi/pal_abi.h"
#include "xax_network_defs.h"
#include "xax_network_utils.h"
#include "hash_table.h"
#include "ZeroCopyBuf.h"
#include "ZLCEmit.h"

class PacketDispatcherIfc {
public:
	virtual void dispatch_packet(ZeroCopyBuf *zcb, IPInfo *info) = 0;
};

class FragmentReassembly
{
private:
	PacketDispatcherIfc *dispatcher;
	HashTable _ht0, _ht1;
	HashTable *h_new, *h_old;
	MallocFactory *mf;
	SyncFactory *sf;
	ZLCEmit *ze;

	static void remove_func(void *v_this, void *datum);
	enum { MAX_PARTIAL_PACKETS = 2 };
	void tidy();

public:
	FragmentReassembly(PacketDispatcherIfc *dispatcher, MallocFactory *mf, SyncFactory *sf, ZLCEmit *ze);
	void handle_fragment(ZeroCopyBuf *zcb, IPInfo *info);
};
