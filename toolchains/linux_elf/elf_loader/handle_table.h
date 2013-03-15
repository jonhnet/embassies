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

#include "XaxVFS.h"
#include "malloc_factory.h"

class XVFSHandleWrapper;

typedef enum {
	ht_free = 0,
	ht_assigned = 1,
	ht_used = 2,
	ht_notfree = 0x7fffffff,
} HTState;

typedef struct {
	HTState state;
	union {
		XVFSHandleWrapper *wrapper;
		int next_free;
	};
} HandleTableEntry;

class HandleTable : public XaxVFSHandleTableIfc
{
public:
	HandleTable(MallocFactory *mf);

	XVFSHandleWrapper *lookup(int filenum);

	virtual int allocate_filenum();
	virtual void assign_filenum(int filenum, XVFSHandleWrapper *wrapper);
	virtual void assign_filenum(int filenum, XaxVFSHandleIfc *hdl);
	virtual void free_filenum(int filenum);
	virtual void set_nonblock(int filenum);

private:
	MallocFactory *mf;
	int capacity;
	int first_free;
	int counters[3];
	HandleTableEntry *entries;
	CheesyLock entries_lock;

	void _add_to_free_list_lockheld(int filenum);
	void _expand_lockheld();
	void _transition_lockheld(int filenum, HTState s0, HTState s1);
};
