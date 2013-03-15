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

#include "zarfile.h"

// This class factors out the knowledge of how to decode the chdr in a
// Zarfile, so I can use the same decoding logic in both the real
// client (elf_loader/ZarfileVFS.cpp) and the test tool (zarfile_tool).
class ChunkDecoder {
protected:
	ZF_Zhdr* zhdr;
	ZF_Phdr* phdr;
public:
	ChunkDecoder(ZF_Zhdr* zhdr, ZF_Phdr* phdr) : zhdr(zhdr), phdr(phdr) {}
	virtual void underlying_zarfile_read(
		void *out_buf, uint32_t len, uint32_t offset) = 0;
	virtual void read_chdr(int local_chunk_idx, ZF_Chdr *out_chdr);
		// default impl uses CHdrs from zarfile; overridden
		// by ZarfileHandleChunkDecoder to use locally-modified CHdr info.
	void read(void *buf, uint32_t count, uint32_t offset);
};
