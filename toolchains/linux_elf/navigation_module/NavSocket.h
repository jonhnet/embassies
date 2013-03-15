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

#include "NavInstance.h"

class NavSocket {
private:
	void _lookup_addrs();
	void _open_socket();

protected:
	ZoogDispatchTable_v1 *zdt;
	XIPAddr local_interface;
	XIPAddr broadcast;

	int socketfd;
    struct sockaddr_in sender_address;
    socklen_t sender_address_len;

public:
	NavSocket();
	~NavSocket();

	void send_and_delete_msg(NavigationBlob *msg);
	ZoogDispatchTable_v1 *get_zdt() { return zdt; }
};

