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

#include <cheesylock.h>
#include <zutex_sync.h>
#include <pal_abi/pal_abi.h>

#include "hash_table.h"
#include "xax_network_utils.h"	// IPInfo
#include "xax_skinny_network_placebo.h"
#include "MessageQueue.h"

enum { IPVER_ANY = 0 };

typedef struct s_pfregistration {
	uint8_t ipver;
	uint8_t ip_protocol;
	uint16_t udp_port;
} PFRegistration;

typedef struct s_pfhandler {
	PFRegistration registration;
	MessageQueue mq;
	XaxSkinnyNetworkPlacebo *xsn;
} PFHandler;

PFHandler *pfhandler_init(XaxSkinnyNetworkPlacebo *xsn, uint8_t ipver, uint8_t ip_protocol, uint16_t udp_port);
void pfhandler_free(PFHandler *ph);

typedef struct {
	MallocFactory *mf;
	CheesyLock pt_mutex;
	HashTable ht;
} PFTable;

void pftable_init(PFTable *pt, MallocFactory *mf);
void pftable_insert(PFTable *pt, PFHandler *ph);
void pftable_remove(PFTable *pt, PFHandler *ph);
PFHandler *pftable_lookup(PFTable *pt, PFRegistration *pr);
