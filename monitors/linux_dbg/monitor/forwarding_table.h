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

#include "linked_list.h"
#include "pal_abi/pal_net.h"
#include "PacketQueue.h"

struct s_netifc;
typedef void (DeliverFunc)(struct s_netifc *ni, PacketIfc *p, XIPAddr *dest);

typedef struct {
	XIPAddr dest;
	XIPAddr mask;
	DeliverFunc *deliver_func;
	const char *deliver_dbg_name;
} ForwardingEntry;

typedef struct {
	LinkedList list;
} ForwardingTable;

void forwarding_table_init(ForwardingTable *ft, MallocFactory *mf);
void forwarding_table_add(ForwardingTable *ft, XIPAddr *dest, XIPAddr *mask, DeliverFunc *deliver_func, const char *deliver_dbg_name);
	// insert at beginning of routing table.
void forwarding_table_remove(ForwardingTable *ft, XIPAddr *dest, XIPAddr *mask);
ForwardingEntry *forwarding_table_lookup(ForwardingTable *ft, XIPAddr *addr);

void forwarding_table_print(ForwardingTable *ft);
