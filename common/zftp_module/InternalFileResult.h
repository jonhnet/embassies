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

#include "DataRange.h"
#include "ZeroCopyBuf.h"
#include "Buf.h"

class ZCachedFile;
class ZFileRequest;

class InternalFileResult {
public:
	InternalFileResult(ZCachedFile *zcf);

	virtual uint32_t get_reply_size(ZFileRequest *req) = 0;
		// this call should be followed by exactly one of the next call;
		// this object saves between the size & fill calls.
	virtual void fill_reply(Buf* buf, ZFileRequest *req) = 0;

//	virtual ZeroCopyBuf *build_reply(DataRange *req_set) = 0;
	virtual bool is_error() = 0;

protected:
	ZCachedFile *zcf;
};
