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
#ifndef _ZFTP_CLIENT_H
#define _ZFTP_CLIENT_H

#include <stdint.h>
#include "zftp_network_state.h"
#include "xax_network_defs.h"
#include "malloc_trace.h"

typedef struct s_zftp_block
{
	uint32_t block_index;
	uint8_t *buf_to_free;
	uint8_t *data;	// points into buf's guts
	MallocFactory *mf;
	MallocTraceFuncs *mtf;	// zftp block buffer leak detection
} ZFTPBlock;

typedef struct
{
	ZFTPNetworkState *netstate;

	UDPEndpoint local_ep;
	UDPEndpoint remote_ep;

	// request components
	hash_t file_hash;
	char *url_hint;

	// learned after first reply; verified with all future replies
	bool know_file_data;
	size_t file_len;
	size_t num_blocks;

	uint32_t err;

	ZFTPBlock **blocks;

	MallocTraceFuncs *mtf;
} ZFTPClientState;

bool
zftp_get(ZFTPNetworkState *netstate, hash_t file_hash, const char *url_hint, size_t *out_size, ZFTPFileMetadata *metadata, uint8_t **contents);
	// Contents may be NULL, in which case contents won't be copied to
	// caller (and hence caller won't be responsible to discard them).
	// (Nor will anything past first block be fetched.)
	// false => file didn't exist (and fcn assigns *contents = NULL)

ZFTPBlock *zftp_fetch_block(ZFTPClientState *state, uint32_t block_num, bool dev_insecure_hash_lookup_mode, ZFTPFileMetadata *metadata);
	
hash_t
dev_insecure_hash_lookup(ZFTPNetworkState *netstate, char *url_hint);

void zftp_block_free(ZFTPBlock *zb);

ZFTPClientState *
zftp_connection_setup(ZFTPNetworkState *netstate, hash_t file_hash, const char *url_hint, MallocTraceFuncs *mtf);

void ZFTPClientState_free(ZFTPClientState *state);

void set_zero_hash(hash_t *hash);


#endif // _ZFTP_CLIENT_H
