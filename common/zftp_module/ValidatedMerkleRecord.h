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

typedef enum {
	v_unknown = 0,
	v_tentative,
	v_validated,
	v_margin
		// v_margin used to help with asserts in ZOriginFile
} ValidityState;


class Avl_uint32_t {
private:
	uint32_t value;
public:
	void set_value(uint32_t value) { this->value = value; }
	inline uint32_t getKey() { return value; }
};


class ValidatedMerkleRecord {
public:
	static uint32_t __hash__(const void *a);
	static int __cmp__(const void *a, const void *b);
	
	Avl_uint32_t _location;
		// seems silly to store location, since that's implied by
		// position in array, but it's a way to put a bunch of
		// these in a linked list and cleanly recover the location
		// value without a bunch of heinous pointer math.
		// Or in a tree.

public:
	hash_t hash;
	ValidityState state;
	void set_location(uint32_t loc) { _location.set_value(loc); }
	inline uint32_t location() { return _location.getKey(); }
	inline Avl_uint32_t *get_avl_elt() { return &_location; }
};
