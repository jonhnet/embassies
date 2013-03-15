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

class ShmMapping {
private:
	uint32_t size;
	int key;		// the system-wide visible name for this segment
	int shmid;		// the process-local handle for this segment
	void *mapping;

public:
	ShmMapping(uint32_t size, int key, int shmid);
	ShmMapping(void *mapping);

	uint32_t get_size() { return size; }
	int get_key() { return key; }
	void *map(void *req_addr);
	void *map() { return map(NULL); }
	void unmap();
	void unlink();
};

class Shm {
private:
	uint32_t uniquifier;
		// the shm namespace is 32 bits, machine-wide. Insane(-ly stupid)
	uint32_t next_identity;

	uint32_t _make_uniquifier(int tunid);

public:
	Shm(int tunid);
	ShmMapping allocate(uint32_t identity, uint32_t size);
	ShmMapping allocate(uint32_t size);
};
