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
#error dead code
#pragma once

#include "Emittable.h"

class CatalogEntry;
class StringIndex;

class ZIndex : public Emittable {
private:
	int num_entries;
	CatalogEntry **ce_array;
	StringIndex *string_index;

public:
	ZIndex(int num_entries, StringIndex *string_index);
	virtual ~ZIndex();
	uint32_t get_path_count() { return num_entries; }

	virtual uint32_t get_size();
	virtual void emit(FILE *fp);

	void assign(int slot, uint32_t data_offset, CatalogEntry *ce);
};
