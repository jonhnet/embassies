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

#include "InternalFileResult.h"
#include "linked_list.h"

class ValidFileResult : public InternalFileResult {
public:
	ValidFileResult(ZCachedFile *zcf);

	uint32_t get_reply_size(ZFileRequest *req);
	void fill_reply(Buf* vbuf, ZFileRequest *req);
	bool is_error();

	uint32_t get_filelen();
	DataRange get_file_data_range();
	
//	void read_set(DataRange range, uint8_t *buf, uint32_t size);
	void write_payload_to_buf(Buf* vbuf, uint32_t data_offset, uint32_t size);
		// ZFetch uses this interface.

	bool is_dir();

private:
	DataRange get_available_data_range(ZFileRequest *req);
};
