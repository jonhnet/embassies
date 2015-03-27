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
#include "Hashable.h"
#include "zftp_protocol.h"

class HashableHash : public Hashable {
private:
	hash_t _hash;
	uint32_t _found_at_offset;
	uint32_t _n_occurrences;

public:
	HashableHash(hash_t *hash, uint32_t found_at_offset);
	uint32_t hash();
	int cmp(Hashable *other);
	void increment();
	uint32_t get_found_at_offset();
	uint32_t get_num_occurrences();
	hash_t* get_hash() { return &_hash; }
};
