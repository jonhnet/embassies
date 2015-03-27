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

#include "PHdr.h"
#include "StringTable.h"
#include "ChunkTable.h"
#include "zarfile.h"

class ZHdr : public Emittable {
private:
	ZF_Zhdr zhdr;

	PHdr* _phdr0;
	uint32_t _index_count;
	StringTable* _string_table;
	ChunkTable* _chunk_table;

public:
	ZHdr(uint32_t zftp_lg_block_size);
	void set_dbg_zarfile_len(uint32_t dbg_zarfile_len);

	virtual uint32_t get_size();

	void connect_index(PHdr *phdr) { _phdr0 = phdr; }
	void set_index_count(uint32_t index_count) { _index_count = index_count; }
	void connect_string_table(StringTable *string_table) { _string_table = string_table; }
	void connect_chunk_table(ChunkTable *chunk_table) { _chunk_table = chunk_table; }

	virtual const char* get_type();
	virtual void emit(FILE *fp);
};
