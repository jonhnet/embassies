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

#include <stdbool.h>
#include "pal_abi/pal_abi.h"
#include "zutex_sync.h"
#include "malloc_factory.h"

typedef bool (consume_method_f)(void *user_storage, void *user_request);
	// Method reads a user_request to remove some stuff from user_storage.
	// If this unit of user_storage is used up, free it and return
	// 'true' to have it removed from queue head.

typedef struct s_zqentry {
	struct s_zqentry *next;
	void *user_storage;
} ZQEntry;

typedef struct s_zqueue {
	ZAS zas;
	ZMutex zmutex;
	uint32_t seq_zutex; 
	ZQEntry head;
	ZQEntry *tailp;
	consume_method_f *consume_method;
	ZoogDispatchTable_v1 *xdt;
	MallocFactory *mf;
} ZQueue;

struct s_zqueue *zq_init(ZoogDispatchTable_v1 *xdt, MallocFactory *mf, consume_method_f *consume_method);
void zq_free(ZQueue *);
void zq_insert(ZQueue *zq, void *user_storage);
void zq_consume(ZQueue *zq, void *consume_request);
	// Provide a consume_request block in a format understood
	// by your consume_method. Oh, man, I wish I had me some OOP.
bool zq_is_empty(ZQueue *zq);
ZAS *zq_get_zas(ZQueue *zq);
