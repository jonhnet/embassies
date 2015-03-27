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

#include "pal_abi/pal_types.h"

class DataRange {
public:
	DataRange();
	DataRange(uint32_t start, uint32_t end);
	void accumulate(uint32_t start, uint32_t end);
	void accumulate(DataRange other);
	bool maybe_accumulate(uint32_t start, uint32_t end);
	bool maybe_accumulate(DataRange other);
	uint32_t start() { return _start; }
	uint32_t end() { return _end; }
	uint32_t size() { return _end - _start; }
	DataRange intersect(DataRange other);
	bool equals(DataRange *other);

	inline bool is_empty() { return _start==_end; }
	bool intersects(DataRange other) { return !intersect(other).is_empty(); }

private:
	uint32_t _start;
	uint32_t _end;
};
