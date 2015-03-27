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
#include "Emittable.h"
#include "CatalogEntry.h"
#include "ChunkEntry.h"

class ChunkTable : public Emittable {
private:
	int total_size;
	int chunk_index;
	LinkedList chunks;

public:
	ChunkTable(MallocFactory *mf);
	virtual ~ChunkTable();
	uint32_t allocate(ChunkEntry *chunk_entry);

	virtual uint32_t get_size();
	virtual const char* get_type();
	virtual void emit(FILE *fp);

	uint32_t get_count();
};
