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

#include "CoalescingAllocator.h"

#define PAGE_SIZE	(0x1000)

class MemSlot : public UserObjIfc {
public:
	MemSlot(const char *dbg_label);
	~MemSlot();
	
	const char *get_dbg_label();
	uint32_t get_guest_addr();
	uint32_t get_size();
	uint8_t *get_host_addr();
	void configure(Range guest_range, uint8_t *host_addr);
	Range get_guest_range() { return guest_range; }

	static uint32_t round_up_to_page(uint32_t req_size);
	static uint32_t round_down_to_page(uint32_t req_size);

private:
	const char *dbg_label;
	bool configured;
	Range guest_range;
	uint8_t *host_addr;
};
