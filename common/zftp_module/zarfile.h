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
#include "zftp_protocol.h"

#define Z_MAGIC		((uint32_t) (((int*)"ZARF")[0]))
#define Z_VERSION	((uint32_t) (0x00010002))

#define ZFTP_METADATA_FLAG_ENOENT	(2)
	// Set this flag in protocol_metadata.flags to signal a tombstone (a
	// name maps positively to ENOENT; don't fall through to an underlying
	// source of files).

typedef struct {
	uint32_t z_magic;
	uint32_t z_version;
	uint32_t z_path_num;
	uint32_t z_path_off;
	uint32_t z_path_entsz;
	uint32_t z_strtab_off;
	uint32_t z_strtab_len;
	uint32_t z_dbg_zarfile_len;
	uint32_t z_chunktab_off;		// offset to ZF_Chdr *. 0 => ENOENT
	uint32_t z_chunktab_count;		// length of ZF_Chdr array in entries (not bytes)
	uint32_t z_chunktab_entsz;		// sizeof(ZF_Chdr)
} ZF_Zhdr;

typedef struct {
	uint32_t z_path_str_off;	// offset into strtab (not beginning of file)
	uint32_t z_path_str_len;	// for implementations that read strtab incrementally.
	uint32_t z_file_len;		// logical length of file, regardless of which data chunks are available
		// (host-order version of protocol_metadata.file_len_network_order)
	uint32_t z_chunk_idx;		// index into ZF_Chdr[]
	uint32_t z_chunk_count;		// number of chunks in ZF_Chdr for this file
		// NB a file may have overlapping chunks; that is, subranges
		// of its data may appear multiple times in the Chdrs. This is
		// in support of (fairly-) efficiently servicing mmap requests
		// without memcpy.
	ZFTPFileMetadata protocol_metadata;
} ZF_Phdr;

typedef struct {
	uint32_t z_file_off;		// offset into zarfile.
	uint32_t z_data_off;		// offset into requested file of specified chunk
	uint32_t z_data_len;
	bool	 z_precious;		// don't give this chunk away in a fast_mmap.
} ZF_Chdr;

// NB path records must appear in order sorted by corresponding path strings
// (with no duplicates); client binary-searches Phdr array.
// NB path strings should be zero-terminated, although zero sentinel doesn't
// count in z_path_str_len.
