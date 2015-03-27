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

#include "avl2.h"
#include "linked_list.h"
#include "Emittable.h"
#include "DataRange.h"
#include "SyncFactory_Pthreads.h"
#include "hash_table.h"

class Placement
{	// sorts by location
private:
	DataRange range;
	Emittable* e;

public:
	Placement(Emittable*e, uint32_t location)
		: range(location,location+e->get_size()), e(e) {}
	Placement(DataRange free_range)
		: range(free_range), e(NULL) {}
	int cmp(Placement *a, Placement *b);
	Placement getKey() { return *this; }
	bool is_free() { return e==NULL; }

	bool operator<(Placement b)  { return cmp(this, &b)<0; }
	bool operator>(Placement b)  { return cmp(this, &b)>0; }
	bool operator==(Placement b) { return cmp(this, &b)==0; }
	bool operator<=(Placement b) { return cmp(this, &b)<=0; }
	bool operator>=(Placement b) { return cmp(this, &b)>=0; }
	DataRange get_range() { return range; }
	Emittable* get_emittable() { return e; }
};

class FreeToken
{	// sorts by size
private:
	DataRange range;
public:
	FreeToken(DataRange range) : range(range) {}
	int cmp(FreeToken *a, FreeToken *b);
	FreeToken getKey() { return *this; }

	bool operator<(FreeToken b)  { return cmp(this, &b)<0; }
	bool operator>(FreeToken b)  { return cmp(this, &b)>0; }
	bool operator==(FreeToken b) { return cmp(this, &b)==0; }
	bool operator<=(FreeToken b) { return cmp(this, &b)<=0; }
	bool operator>=(FreeToken b) { return cmp(this, &b)>=0; }
	DataRange get_range() { return range; }
};

typedef AVLTree<Placement,Placement> PlacementTree;
typedef AVLTree<FreeToken,FreeToken> FreeTree;


class Placer {
private:
	MallocFactory* _mf;
	uint32_t zftp_block_size;
	uint32_t total_size;
	uint32_t dbg_total_padding;
	SyncFactory_Pthreads sf;
	PlacementTree* placement_tree;
	FreeTree* free_tree;

	FILE *ofp;
	uint32_t current_offset;
	HashTable dbg_type_ht;
	FILE* debugf;

	void stretch(uint32_t size);
	void delete_all();
	void dbg_sanity();
//	void emit_zeros(FILE* ofp, uint32_t size);
	void emit_one(Emittable* e);

	static void _dbg_print_one_type(void* v_this, void* v_a);
	void dbg_bytes_by_type(uint32_t total_bytes);

public:
	Placer(MallocFactory *mf, uint32_t zftp_block_size);
	~Placer();
	uint32_t gap_size();
	uint32_t get_offset();
	
	void add_space(uint32_t size);
	bool place(Emittable *e, uint32_t location);
	void place(Emittable *e);
	void pad(uint32_t padding_size);
	void emit(const char *out_path);
	uint32_t get_total_size() { return total_size; }

	void dbg_report();
};
