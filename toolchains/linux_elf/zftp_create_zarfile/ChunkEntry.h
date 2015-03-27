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

#include "Emittable.h"
#include "zarfile.h"
#include "FileSystemView.h"

class CatalogEntry;

class ChunkEntry : public Emittable {
private:
	ZF_Chdr _chdr;
	CatalogEntry *catalog_entry;

public:
	enum ChunkType { PRECIOUS, FAST };
	ChunkEntry(uint32_t offset, uint32_t len, ChunkType chunk_type, CatalogEntry *catalog_entry);
	~ChunkEntry();
	const char* get_url();	// minor sort key, after size.
	virtual void set_location(uint32_t offset);

	ZF_Chdr* get_chdr();
	virtual uint32_t get_size();
	virtual const char* get_type();
	virtual void emit(FILE *fp);
	virtual void emit(WriterIfc* writer);
};
