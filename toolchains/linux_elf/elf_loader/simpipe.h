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

#include "linked_list.h"

class SimParcel
{
public:
	SimParcel(MallocFactory *mf, const void *buf, size_t count);
	~SimParcel();

	void *data;
	void *buffer_to_free;
	size_t count;
	MallocFactory *mf;
};

class SimPipe;

class SimHandle : public XaxVFSHandlePrototype
{
public:
	SimHandle(MallocFactory *mf, ZoogDispatchTable_v1 *xdt, SimPipe *simpipe);
	~SimHandle();

	virtual bool is_stream() { return true; }
	virtual uint64_t get_file_len() { lite_assert(false); return 0; }

	virtual uint32_t read(
		XfsErr *err, void *dst, size_t len);
	virtual void write(
		XfsErr *err, const void *src, size_t len, uint64_t offset);
	virtual void xvfs_fstat64(
		XfsErr *err, struct stat64 *buf);
	virtual ZAS *get_zas(
		ZASChannel channel);

private:
	MallocFactory *mf;
	ZoogDispatchTable_v1 *xdt;
	SimPipe *simpipe;
};

class SimPipe
{
public:
	SimPipe(MallocFactory *mf, ZoogDispatchTable_v1 *xdt);
	static bool _zas_check(ZoogDispatchTable_v1 *xdt, ZAS *zas, ZutexWaitSpec *out_spec);

public:
	ZAS zas;	// Pole-position matters; we compute SimPipe*==ZAS*
	uint32_t zutex;
	ZMutex zmutex;

	SimHandle *readhdl;
	SimHandle *writehdl;
	LinkedList parcels;
	ZAlwaysReady always_ready;

private:
	static ZASMethods3 simpipe_zas_methods;
};
