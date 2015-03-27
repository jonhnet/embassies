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
#include "zftp_protocol.h"
#include "Buf.h"

class MemBuf : public Buf {
public:
	MemBuf(uint8_t* buffer, uint32_t buf_len);
	// Buffer we represent

	virtual void write_data(ZBlockCacheRecord* block, uint32_t data_offset, uint32_t block_offset, uint32_t len);
	virtual uint32_t get_payload_len();

protected:
	uint8_t* get_data_buf();

private:
	uint8_t* buffer;
	uint32_t buf_len;
	uint32_t max_data_len;
};
