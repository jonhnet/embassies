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
#include <stdio.h>
#include "ChunkDecoder.h"

class PosixChunkDecoder : public ChunkDecoder {
private:
	FILE* ifp;
public:
	PosixChunkDecoder(ZF_Zhdr* zhdr, ZF_Phdr* phdr, FILE* ifp)
		: ChunkDecoder(zhdr, phdr), ifp(ifp) {}
	virtual void underlying_zarfile_read(
		void *out_buf, uint32_t len, uint32_t offset);
};


