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
#ifndef _PACKETQUEUE_H
#define _PACKETQUEUE_H

#include <stdio.h>
#include <netinet/in.h>
#include <pthread.h>

#include "hash_table.h"
#include "EventSynchronizer.h"
#include "Packets.h"
#include "linked_list.h"

typedef struct {
	pthread_mutex_t mutex;
	HashTable outstanding_packets;
	int total_alloc;
	int total_free;
	FILE *packet_history;
} PacketAccount;

/*
Packet *packet_init(char *data, int len, PacketAccount *pa, const char *purpose);
Packet *packet_alloc(int len, PacketAccount *pa, const char *purpose);
Packet *packet_copy(Packet *p, PacketAccount *pa, const char *purpose);
void packet_free(Packet *packet, PacketAccount *pa);
char *packet_data(Packet *p);
*/

void packet_account_init(PacketAccount *pa, MallocFactory *mf);
void packet_account_create(PacketAccount *pa, PacketIfc *p);
void packet_account_free(PacketAccount *pa, PacketIfc *p);

typedef struct {
	LinkedList queue;
	pthread_mutex_t mutex;
#if 0
	pthread_cond_t cond;
#endif
	EventSynchronizer *event_synchronizer;
	uint32_t sequence_number;
	uint32_t size_bytes;
	PacketAccount *packet_account;

	uint32_t max_bytes;
	int min_packets;
} PacketQueue;

void pq_init(PacketQueue *pq, PacketAccount *packet_account, uint32_t max_bytes, int min_packets, EventSynchronizer *event_synchronizer);
void pq_insert(PacketQueue *pq, PacketIfc *p);
PacketIfc *pq_peek(PacketQueue *pq);
PacketIfc *pq_remove(PacketQueue *pq);
uint32_t pq_block(PacketQueue *pq, uint32_t known_sequence_number);
void pq_tidy(PacketQueue *pq, uint32_t max_bytes, uint32_t min_packets);

#endif // _PACKETQUEUE_H
