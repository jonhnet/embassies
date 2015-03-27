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
#ifndef _ZFTP_PROTOCOL_H
#define _ZFTP_PROTOCOL_H

#include "pal_abi/pal_types.h"
#include "crypto.h"

// memcmp
#include <string.h>
/*
#include <stdint.h>
*/

/*
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
*/
#include "LiteLib.h"

#define ZFTP_METADATA_FLAG_ISDIR	1

#pragma pack(push)
#pragma pack(1)
typedef struct
{
	uint32_t file_len_network_order;
	uint32_t flags;

	// We define file_hash = hash[ ZFTPFileMetadata : root_hash ].
} ZFTPFileMetadata;
	// This data structure must be kept in network order, even though
	// we use it internally as well, because it's used in generating
	// the file_hash, which must be in a canonical representation.
#pragma pack(pop)

// revised protocol: the new format is stateless, and allows the client
// to fetch blocks mid-file. The client can say how much of the merkle
// tree path to the root it needs. The lazy client will always ask for
// the whole path; a cleverer client can build up a cache of the tree and
// save some overhead (1.4% with a binary merkle tree).
// We're assuming a binary merkle tree here as a shortcut.

#define ZFTP_HASH_LEN	(32)
#define ZFTP_PORT	(7662)
#define ZFTP_LG_BLOCK_SIZE	(16)
#define ZFTP_BLOCK_SIZE (1<<ZFTP_LG_BLOCK_SIZE)
#define ZFTP_TREE_DEGREE	(2)

#define FILE_SCHEME	"file:"
#define STAT_SCHEME	"stat:"
	// special shadow namespace for fetching unix stat information about
	// corresponding file: paths. The file: only provides ZFTP length
	// and the actual content bytes.

#define ZFTP_MAX_BLOCK_COUNT	(0xffffffff)	/* todo uint32_t */

#pragma pack(push)
#pragma pack(1)

typedef struct s_compression_context
{
	uint32_t state_id;
	uint32_t seq_id;
} CompressionContext;
#define ZFTP_NO_COMPRESSION		(0)
// set state_id == ZFTP_NO_COMPRESSION for no compression.

// If the client requests a (state_id,seq_id) for which the server doesn't
// hold the corresponding compression state, then it will return something
// with (state_id,0) -- it'll start a new compression state at state_id.

typedef struct s_zftp_request_packet
{
	uint16_t hash_len;
	uint16_t url_hint_len;

	hash_t file_hash;

	uint32_t num_tree_locations;

	uint32_t padding_request;

	CompressionContext compression_context;

	uint32_t data_start;
	uint32_t data_end;
		// request file data from [data_start,data_end)

	// This struct followed by a variable-length region containing:
	// <tree_locations> num_tree_locations*uint32_t requested tree locations
	// <url_hint> url_hint_len*char string. Last byte must be 0.
} ZFTPRequestPacket;
#pragma pack(pop)

typedef uint16_t ZFTPReplyCode;
enum {
	zftp_reply_data = 0x0000,
	zftp_reply_hint_required = 0x0101,
	zftp_reply_error_url_not_found = 0x0102,
	zftp_reply_error_other = 0x0103,
	zftp_reply_error_incorrect_hash_for_url = 0x0104,
	zftp_reply_error_yeah_that_is_zero = 0x0105,
};

#pragma pack(push)
#pragma pack(1)
typedef struct
{
	ZFTPReplyCode code;
} ZFTPReplyHeader;

typedef struct s_zftp_merkle_record
{
	uint32_t tree_location;	// < tree_size
	hash_t hash;
} ZFTPMerkleRecord;

typedef struct s_zftp_data_packet
{
	ZFTPReplyHeader header;

// protocol properties. Right now, these are constants, and likely
// will remain that way. Note that changing zftp_tree_degree!=2 would
// require redefining sibling hash fields.
	uint16_t 	hash_len;
	uint8_t 	zftp_lg_block_size;
	uint16_t	zftp_tree_degree;

// file properties
	hash_t file_hash;
	ZFTPFileMetadata metadata;

// packet properties
	uint16_t num_merkle_records;
		// The 'merkle_records' portion of this packet contains
		// num_merkle_records instances of ZFTPMerkleRecord.

	uint32_t padding_size;

	CompressionContext compression_context;

	uint32_t data_start;
	uint32_t data_end;
		// The 'data' portion of this packet represents
		// file data from [data_start,data_end).
		// (possibly with trailing zeros omitted).
	uint32_t payload_size;

	// This struct followed by a variable-length region containing:
	// <merkle_records>
	//      num_merkle_records * sizeof(ZFTPMerkleRecord) bytes of tree hashes
	//      These should probably be presented in the same order the list
	//      was requested by the client.
	// <padding_size> as many zeros as indicated in uint32_t padding_size;
	// if (compression_context.state_id==ZFTP_NO_COMPRESSION) then
	// 	<data>
	//      	payload_size bytes of raw data, no more than data_end-data_start
	//       	Any truncation implies remaining bytes are 0s.
	// else
	//	<payload_size> bytes of data to feed into zlib.
	//	(decompressor must output (data_end-data_start) bytes.)
	//  compression_context identifies a stream state;
	//	if compression.context.seq_id>0,
	//  that stream continues from a prior packet with same state_id.
} ZFTPDataPacket;
#pragma pack(pop)

#define ZNTOH(v)	(sizeof(v)==sizeof(uint32_t) \
						?z_ntohl(v) \
						:(sizeof(v)==sizeof(uint16_t) \
							?z_ntohs(v) \
							:(v)))

typedef struct
{
	ZFTPReplyHeader header;
	uint16_t message_len;
	hash_t file_hash;

	// This struct followed by a variable-length region containing:
	// message_len bytes of error message. Last byte must be 0.
} ZFTPErrorPacket;

/*
 * bytes:blocks:tree_level relationships
 */
static __inline uint32_t compute_num_blocks(uint32_t file_len)
{
	return ((file_len)+ZFTP_BLOCK_SIZE-1) >> ZFTP_LG_BLOCK_SIZE;
}

// This is a 
static __inline uint16_t
ceillg2(uint32_t n)
{
	uint16_t r;
	for (r=0; n>0; r++) { n=n>>1; }
	return r;
}

static __inline uint16_t
compute_tree_depth(uint32_t num_blocks)
	// NB tree_depth is address of bottom layer of tree
{
	if (num_blocks==0) { return 0; }
	return ceillg2(num_blocks-1);
}

#if 0	// dead code from a dumber version of the protocol
static __inline uint16_t
compute_num_merkle_siblings(uint16_t first_merkle_sibling, uint32_t file_len)
{
	uint16_t tree_depth = compute_tree_depth(compute_num_blocks(file_len));
	return tree_depth-(first_merkle_sibling-1);
}
#endif

// defined for depth>=0
static __inline uint32_t
compute_tree_size(uint16_t depth)
{
	return (1<<(depth+1))-1;
}

static __inline uint32_t
compute_tree_location(int level, int col)
{
	lite_assert(col < (1<<level));
	if (level==0)
	{
		return 0;
	}
	uint32_t index = compute_tree_size(level-1)+col;
	return index;
}

/*
	num_blocks = 4 => tree_depth = 3.
	Such a tree has a root level (0), with one hash
		("root_hash", part of requested file_hash),
	a level 1 with 2 hashes,
	and a level 2 with 4 hashes, each of a data block.
	tree_depth is a function of file_len
	file_len == 0 => tree_depth == 0
	file_len == 1 => tree_depth == 0
	file_len == ZFTP_BLOCK_SIZE-1 => tree_depth == 0
	file_len == ZFTP_BLOCK_SIZE   => tree_depth == 0
	file_len == ZFTP_BLOCK_SIZE+1 => tree_depth == 1
	...
*/

static __inline int cmp_hash(const hash_t *h0, const hash_t *h1)
{
	return memcmp((uint8_t*) h0, (uint8_t*) h1, sizeof(hash_t));
}

static __inline bool is_zero_hash(const hash_t *hash)
{
	return lite_memequal((const char*) hash, 0, sizeof(hash_t));
}

static __inline hash_t get_zero_hash()
{
	hash_t z;
	lite_memset(&z, 0, sizeof(z));
	return z;
}

#endif // _ZFTP_PROTOCOL_H
