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

#include "malloc_factory.h"
#include "linked_list.h"
#include "hash_table.h"
#include "zftp_protocol.h"
#include "xax_network_utils.h"

#include "ZFileRequest.h"
#include "ZBlockCacheRecord.h"
#include "ValidatedMerkleRecord.h"
#include "ZDBDatum.h"
#include "ZCachedStatus.h"
#include "SyncFactory.h"
#include "ZLCEmit.h"
#include "ZFileReplyFromServer.h"
#include "SendBufferFactory.h"
#include "RequestBuilder.h"
#include "ZStats.h"
#include "Buf.h"

class ZFileDB;
class ZBlockDB;

class ZCache;

class ZCachedFile {
public:
	ZCachedFile(ZCache *zcache, const hash_t *file_hash);
	ZCachedFile(const hash_t *file_hash);	// key ctor
	~ZCachedFile();

	void consider_request(ZFileRequest *req);
	void withdraw_request(ZFileRequest *req);

	void install_reply(ZFileReplyFromServer *reply);

	static uint32_t __hash__(const void *a);
	static int __cmp__(const void *a, const void *b);

	void marshal(ZDBDatum *v);
	ZCachedFile(ZCache *cache, ZDBDatum *v);	// unmarshal ctor

	uint32_t get_filelen() { return Z_NTOHG(metadata.file_len_network_order); }

	DataRange get_file_data_range();
		// asserts metadata known; returns DataRange covering file len

	ValidatedMerkleRecord *get_validated_merkle_record(uint32_t location);

	void read(Buf* buf, uint32_t buf_offset, uint32_t offset, uint32_t size);
		// asserts required blocks are present

	bool is_dir();

	SendBufferFactory *get_send_buffer_factory();

	uint32_t get_progress_counter() { return progress_counter; }


protected:
	ZCachedFile(ZLCEmit *ze) { this->ze = ze; }
	// polymorphic behavior that changes in ZOriginFile.
	virtual void _track_parent_of_tentative_nodes(ValidatedMerkleRecord *parent);
	virtual void _assert_root_validated();
	virtual void _mark_node_on_block_load(uint32_t location);

	void _ctor_init(ZCache *zcache, const hash_t *file_hash);
	void _configure_from_metadata();
	bool install_data(ZFileReplyFromServer *reply);
	bool install_merkle_records(ZFileReplyFromServer *reply);
	void _mark_node_tentative(uint32_t location);
	hash_t _compute_file_hash(ZFTPFileMetadata *metadata, hash_t *root_hash);

	void _install_into(ZCachedFile *target);

protected:
	void _common_init(ZCache *zcache);

	// a request arrives
	hash_t* find_block_hash(uint32_t block_num, ValidityState required_validity);
	ZBlockCacheRecord *_lookup_block(uint32_t block_num, ValidityState required_validity);
	void _consider_request_nolock(ZFileRequest *req);

	// if data is required, we issue a request to the server
	DataRange _compute_missing_set(DataRange requested_range);
	void _issue_server_request(
		DataRange missing_set,
		ZFileRequest *waiting_req);

	// upon receipt of data from the server, we fold it into our tree
	// and wake up waiting client requests
	ZFTPMerkleRecord *_find_root_merkle_record_idx(ZFileReplyFromServer *reply);
	enum InstallResult { FAIL, NO_EFFECT, NEW_DATA };
	InstallResult _install_metadata(ZFileReplyFromServer *reply);
	void _mark_hash_validated(ValidatedMerkleRecord *vmr);
	InstallResult _validate_children(ValidatedMerkleRecord *parent);
	InstallResult _validate_tentative_hashes();
	void _hash_children(uint32_t ca, uint32_t cb, hash_t *hash_out);
	bool _load_hash(ZFTPMerkleRecord *record);
	void _load_block(
		uint32_t block_num, uint8_t *in_data, uint32_t valid_data_len);
	void _signal_waiters();

	// and then we form a reply to our client.
#if 0
	static void _add_sibling(void *v_this, void *datum);
	static void _enumerate_siblings(void *v_this, void *datum);
	friend class ValidFileResult;
	LinkedList *_compute_required_siblings(DataRange *set);
#endif
	void _error_reply(ZFileRequest *req, ZFTPReplyCode code, const char *msg);

	//
	void _install_image(ZCachedFile *image, ZCachedStatus new_status);
	void _update_status(ZCachedStatus new_status);

	uint32_t round_up_to_block(uint32_t pos);

public:
	ZCache *zcache;
	ZLCEmit *ze;

	// ground truth
	hash_t file_hash;
	ZFTPFileMetadata metadata;
	ValidatedMerkleRecord *tree;

	// values derived from metadata
	uint32_t file_len;
	uint32_t num_blocks;
	uint16_t tree_depth;	// NB tree_depth is address of bottom layer of tree
	uint32_t tree_size;
	ValidityState metadata_state;

	MallocFactory *mf;
	LinkedList waiters;

	SyncFactoryMutex *mutex;

	// temporary variable used during installation of a packet from a server
	HashTable parents_of_tentative_nodes;
	// temporary variables used in zcachedfile_validate_tentative_hashes
	ValidatedMerkleRecord **sorted_parents;
	uint32_t sorted_index;
#if 0
	// temporary variables used in _compute_required_siblings
	HashTable _sibling_tree;
#endif

	bool origin_mode;

private:
	ZCachedStatus status;

	friend class RequestBuilder;

	uint32_t progress_counter;
	ZStats *zstats;
	bool requesting_data_mode;
};

