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
#include "malloc_factory.h"
#include "zcz_args.h"
#include "Emittable.h"
#include "linked_list.h"
#include "FileSystemView.h"
#include "ChunkTable.h"
#include "StringTable.h"
#include "MmapDecoder.h"

class CatalogEntry;
class CatalogEntryByUrl;
class ChunkEntry;
class ChunkBySize;
class Placer;
class ZHdr;
class ChunkCounter;

typedef AVLTree<CatalogEntryByUrl,CatalogEntryByUrl> EntryTree;
typedef AVLTree<ChunkBySize,ChunkBySize> ChunkTree;

class Catalog {
private:
	MallocFactory* mf;
	MmapDecoder* mmap_decoder;
	EntryTree url_tree;
	ChunkTree chunk_tree;
	StringTable* string_table;
	ChunkTable* chunk_table;
	ZArgs* zargs;
	FileSystemView file_system_view;
	Placer* placer;
	ZHdr* zhdr;

	uint32_t dbg_total_padding;

	void enumerate_entries();
	void place_chunks();
	void add_chunk(uint32_t offset, uint32_t len, bool precious, CatalogEntry *ce, ChunkCounter *cc);
	void lookup_elf_ranges(ZFSReader* zf, LinkedList* out_ranges);
	void insert_elf_chunks(ZFSReader* zf, CatalogEntry* ce, ChunkCounter* cc);
	char* compute_mmap_filename(const char* log_paths);

public:
	Catalog(ZArgs *zargs, SyncFactory *sf);
	void add_entry(CatalogEntry *ce);
	void add_record_noent(const char *url);
	bool add_record_one_url(const char *url);
	bool add_record_one_file(const char *url);
	void scan(const char *log_paths, const char *dep_file_name, const char *dep_target_name);
	void emit(const char *out_name);
};
