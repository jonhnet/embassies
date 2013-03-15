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

#include <stdint.h>

class NavigationBlob {
private:
	uint8_t *_buf;
	uint32_t _capacity;
	uint32_t _size;
	uint32_t _read_ptr;
	uint8_t *_dbg_buf;

	void init(uint32_t start_capacity);
	void grow(uint32_t min_capacity);

	enum { DBG_BLOB_TOKEN = 0x44556677 };
//	enum { DBG_MAGIC = 'x' };

public:
	NavigationBlob();
	NavigationBlob(uint32_t start_capacity);
	NavigationBlob(uint8_t *bytes, uint32_t size);
	~NavigationBlob();
	void append(void *data, uint32_t size);
	void append(NavigationBlob *blob);
	void read(void *data, uint32_t size);
	uint8_t *read_in_place(uint32_t size);
	uint8_t *write_in_place(uint32_t size);
	NavigationBlob *read_blob();
	uint32_t size();	// number of bytes remaining to read

//	void dbg_check_magic();
};

