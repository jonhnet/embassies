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
#include "linked_list.h"

class MDRange {
public:
	uint32_t start;
	uint32_t end;
	MDRange(uint32_t start, uint32_t end) : start(start), end(end) {}
	MDRange(MDRange &proto) : start(proto.start), end(proto.end) {}
	bool equals(MDRange &other);
	bool includes(MDRange &other);
	uint32_t len() { return end-start; }
};

class MmapBehavior {
public:
	const char* filename;
	uint32_t aligned_mapping_size;
	uint32_t aligned_mapping_copies;
	LinkedList precious_ranges;

	MmapBehavior();

	static uint32_t hash(const void* datum);
	static int cmp(const void *v_a, const void *v_b);
	void insert_range(MDRange* a_range);
};

class MmapDecoder {
private:
	HashTable ht;
	enum { MAX_FD_TABLE = 256 };
	char* fd_table[MAX_FD_TABLE];
		// todo we're leaking every string in fd_table,
		// every MmapBehavior in ht,
		// and every string in those MmapBehavior objs.

	void parse(const char* debug_mmap_filename);
	void read_fd_line(const char* line);
	void read_map_line(const char* line);
	static void _dbg_dump_foreach(void* v_this, void* v_behavior);
	bool broken;

public:
	MmapDecoder(const char* debug_mmap_filename);
	MmapBehavior* query_file(const char* filename, bool create);
	void dbg_dump();
};
