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

#include "hash_table.h"
#include "Hashable.h"

class Set {
private:
	MallocFactory *mf;
	HashTable ht;

public:
	Set(MallocFactory *mf);
	void insert(Hashable *h);
		// asserts h not a duplicate
	bool contains(Hashable *h);
	Hashable* lookup(Hashable *h);
	uint32_t count();

	Set *intersection(Set *other);
		// new set contains pointers to objects in this set.
	Set *difference(Set *other);
		// new set contains pointers to objects in this set.

	void visit(ForeachFunc* ff, void* user_obj);
};
