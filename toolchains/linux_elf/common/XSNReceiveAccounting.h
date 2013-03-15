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

// This class exists to figure out why we leak ZoogNetBuffers.
// XaxSkinnyNetwork snarfs them as soon as they're available from
// PAL, and then shuttles them into per-port queues. If someone
// ignores their queue, then those ZNBs get ignored and never discarded.
// The case we found with this tool was ZLookupClient leaving a socket
// open, and a parade of duplicate replies (to persistent timeout-driven
// pokes) shows up later, filling the queue, and keeping anything else in
// the system from making progress. (see [2012.02.12] in notes.)

#include "ZeroCopyBuf_Xnb.h"
#include "SyncFactory.h"
#include "hash_table.h"

class XSNReceiveAccounting;

class ZeroCopyBuf_XSNRA : public ZeroCopyBuf_Xnb
{
private:
	XSNReceiveAccounting *xsnra;

public:
	ZeroCopyBuf_XSNRA(ZoogDispatchTable_v1 *xdt, ZoogNetBuffer *znb, XSNReceiveAccounting *xsnra);
	virtual ~ZeroCopyBuf_XSNRA();

	static uint32_t hash(const void *v_a);
	static int cmp(const void *v_a, const void *v_b);
};

class XSNReceiveAccounting {
private:
	SyncFactoryMutex *mutex;
	HashTable ht;
//	int queue_depth;

public:
	XSNReceiveAccounting(MallocFactory *mf, SyncFactory *sf);
	int get_queue_depth() { return ht.count; }
	void add(ZeroCopyBuf_XSNRA *buf);
	void remove(ZeroCopyBuf_XSNRA *buf);
};

