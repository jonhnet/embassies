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

#include "cheesylock.h"
#include "gsfree_lib.h"
#include "hash_table.h"

#include "linux_dbg_port_protocol.h"

class XPBMapping {
public:
	XaxPortBuffer *xpb;
	uint32_t handle;

	virtual ~XPBMapping() {}

	static uint32_t hash(const void *v_a);
	static int cmp(const void *v_a, const void *v_b);
};

class XPBMappingKey : public XPBMapping {
public:
	XPBMappingKey(uint32_t handle);
};

class MMapMapping : public XPBMapping {
private:
	uint32_t length;

public:
	MMapMapping(MapFileSpec *spec);
	~MMapMapping();
};

class ShmMapping : public XPBMapping {
public:
	ShmMapping(ShmgetKeySpec *spec);
	~ShmMapping();
};

class XPBTable {
private:
	HashTable ht;
	int high_water_mark;
	CheesyLock cheesy_lock;

	XPBMapping *_lookup_nolock(uint32_t handle);
	void _insert(XPBMapping *mapping);
	void _discard(uint32_t handle);

public:
	XPBTable(MallocFactory *mf);
	XaxPortBuffer *lookup(uint32_t handle);
	XaxPortBuffer *insert(XPBMapping *mapping, uint32_t old_handle);
	void discard(XaxPortBuffer *xpb);
};

extern bool g_shm_unlink;
