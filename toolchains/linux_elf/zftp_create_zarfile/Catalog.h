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

#include "malloc_factory.h"
#include "ZCZArgs.h"
#include "Emittable.h"
#include "linked_list.h"
#include "FileSystemView.h"
#include "ChunkTable.h"
#include "StringTable.h"
#include "TraceDecoder.h"
#include "CatalogEntryByUrl.h"
#include "ChunkBySize.h"
#include "PlacementIfc.h"

class CatalogEntry;
class ChunkEntry;
class Placer;
class ZHdr;
class ChunkCounter;


class Catalog {
private:
	MallocFactory* mf;
	SyncFactory *sf;
	TraceDecoder* trace_decoder;
	EntryTree url_tree;
	ChunkTree chunk_tree;
	StringTable* string_table;
	ChunkTable* chunk_table;
	ZCZArgs* zargs;
	FileSystemView file_system_view;
	Placer* placer;
	ZHdr* zhdr;
	uint32_t zftp_lg_block_size;
	uint32_t zftp_block_size;

	void enumerate_entries();
	void place_chunks();
	void add_chunk(uint32_t offset, uint32_t len, ChunkEntry::ChunkType chunk_type, CatalogEntry *ce, ChunkCounter *cc);
//	void lookup_elf_ranges(ZFSReader* zf, LinkedList* out_ranges);
//	void insert_elf_chunks(ZFSReader* zf, CatalogEntry* ce, ChunkCounter* cc);
	char* compute_mmap_filename(const char* log_paths);
	char* cons(const char* scheme, const char* path);
	static void s_scan_foreach(void* v_this, void* v_tpr);
	void scan_foreach(TracePathRecord* tpr);
	const char *path_from_url(const char *url);

public:
	Catalog(ZCZArgs *zargs, SyncFactory *sf);
	void add_entry(CatalogEntry *ce);
	void add_record_noent(const char *url);
	bool add_record_one_url(const char *url);
	bool add_record_one_path(const char *path);
	void scan(const char *trace);
	void emit(const char *out_name);
};
