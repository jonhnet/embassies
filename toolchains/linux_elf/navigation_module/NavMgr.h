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

#include <pthread.h>
#include <netinet/in.h>
#include "NavigationProtocol.h"
#include "hash_table.h"
#include "linked_list.h"
#include "pal_abi/pal_abi.h"
#include "SyncFactory_Pthreads.h"
#include "AppIdentity.h"

#include "NavSocket.h"
#include "NavInstance.h"

class NavMgr : public NavSocket {
private:
	NavigateFactoryIfc *navigate_factory;
	EntryPointFactoryIfc *entry_point_factory;

	AppIdentity _identity;

	uint64_t _nonce_allocator;
	HashTable ack_waiters;
	SyncFactory_Pthreads sf;
	pthread_t _dispatch_thread;
	LinkedList nav_instances;

	////  funcs  ///////////////////////////
	
	void _bind_socket();

	static void *_dispatch_loop_s(void *arg);
	void _dispatch_loop();
	void _dispatch_msg(NavigationBlob *msg);

	bool check_recipient_hack(hash_t *dest);
	void handle_navigate_forward(NavigationBlob *msg);
	void handle_navigate_backward(NavigationBlob *msg);
	void nagivate_acknowledge(NavigationBlob *msg);
	NavInstance* find_instance_by_key(DeedKey key);
	void pane_revealed(NavigationBlob *msg);
	void pane_hidden(NavigationBlob *msg);
	uint64_t next_nonce();

public:
	NavMgr(
		NavigateFactoryIfc *navigate_factory,
		EntryPointFactoryIfc *entry_point_factory);
	NavMgr();	// send-only helper

	// for NavInstance
	uint64_t transmit_navigate_msg(Deed outgoing_deed, ZPubKey *vendor_id, AwayAction *aa);
	bool wait_ack(uint64_t rpc_nonce, int timeout_sec);
	hash_t get_my_identity_hash();
};

