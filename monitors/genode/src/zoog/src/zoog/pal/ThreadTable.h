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
/*
 * \brief  a mapping from gs-stashed identifiers back to Genode Thread objects
 * \author Jon Howell
 * \date   2011-12-27
 */

#pragma once

#include <base/thread.h>

#include "standard_malloc_factory.h"
#include "hash_table.h"
#include "ZoogThread.h"

class ThreadTable
{
private:
	HashTable ht;

public:
	ThreadTable();
	void insert_me(ZoogThread *t);
	ZoogThread *lookup_me();
	ZoogThread *remove_me();
	int get_count() { return ht.count; }
	void enumerate(ZoogThread **ary, int capacity);
};
