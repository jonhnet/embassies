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
#include "ZeroCopyBuf.h"
#include "malloc_factory.h"

class ZeroCopyBuf_Memcpy : public ZeroCopyBuf
{
public:
	ZeroCopyBuf_Memcpy(MallocFactory *mf, uint32_t len);
		// send ctor (allocate data)
	ZeroCopyBuf_Memcpy(MallocFactory *mf, uint8_t *data, uint32_t len);
		// receive ctor (existing data)
	~ZeroCopyBuf_Memcpy();
	uint8_t *data();
	uint32_t len();

private:
	uint8_t *_buf;
	uint32_t _len;
	MallocFactory *mf;
};
