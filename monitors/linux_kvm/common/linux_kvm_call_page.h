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

typedef struct {
	uint32_t *guest_address;
		// address of this struct. Here because we deref through a segment
		// selector to find this object, which we'd like to convert back
		// to an "ordinary pointer" to the object, so we can access it
		// with regular (non-segment) code.
	uint32_t dbg_magic;
	uint32_t capacity;
		// number of bytes available in call_struct
	uint8_t call_struct[0];
} ZoogKvmCallPage;

#define ZOOG_KVM_CALL_PAGE_MAGIC	0x2009ca11

// The monitor and pal agree on the layout of the per-vcpu GDT,
// so that the vcpu can use a seg-register deref to find its
// per-vcpu ZoogKvmCallPage.

const uint8_t gdt_access_present = 0x80;
const uint8_t gdt_access_dpl0 = 0x00;
const uint8_t gdt_access_s_nonsystem = 0x10;
const uint8_t gdt_access_type_code_exec_read_accessed = 0x0b;
const uint8_t gdt_access_type_data_read_write_accessed = 0x03;
const uint8_t gdt_access_code =
		  gdt_access_present
		| gdt_access_dpl0
		| gdt_access_s_nonsystem
		| gdt_access_type_code_exec_read_accessed;
	
const uint8_t gdt_access_data =
		  gdt_access_present
		| gdt_access_dpl0
		| gdt_access_s_nonsystem
		| gdt_access_type_data_read_write_accessed;
const uint8_t gdt_granularity_4kpages = 0x80;
const uint8_t gdt_granularity_db_32bit = 0x40;
const uint8_t gdt_granularity_l_not64bit = 0x00;
const uint8_t gdt_granularity_avl_empty = 0x00;
const uint8_t gdt_granularity =
		  gdt_granularity_4kpages
		| gdt_granularity_db_32bit
		| gdt_granularity_l_not64bit
		| gdt_granularity_avl_empty;

enum GDTAssignment {
	gdt_null		= 0,
	gdt_code_flat	= 1,
	gdt_data_flat	= 2,
	gdt_fs_user		= 3,
	gdt_gs_user		= 4,
	gdt_gs_pal		= 5,
	gdt_count		= 6
};
#define GDT_GS_USER_SELECTOR_STR	"$0x20"
#define GDT_GS_PAL_SELECTOR_STR		"$0x28"
// NB must sync string constants with enums;
// string constants used in inline asm where I don't know
// how to fold in compile-time constants.

inline int gdt_index_to_selector(GDTAssignment index)
{
	return (index<<3)
		| (0<<2)		// use GDT, not LDT
		| (0 & 0x3);	// DPL==0.
}

