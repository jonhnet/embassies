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
#include <sys/socket.h>	// socklen_t

#include "CoordinatorProtocol.h"
#include "LongMessageAllocator.h"

class Message
{
public:
	Message(int rendezvous_socket);

	Message(void *payload, uint32_t size);
		// make something that came in from a tunnel
		// look like a co_deliver_packet, ready to forward out to a monitor.

	~Message();

	void *get_payload();
	uint32_t get_payload_size();
	void ingest_long_message(LongMessageAllocator *allocator, CMLongMessage *cm_long);

	const char *get_remote_addr();
	socklen_t get_remote_addr_len();

private:
	char remote_addr[200];
	socklen_t remote_addr_len;
	
	// a message read off the wire
	char *read_buf;
	uint32_t read_buf_size;

	// a long message in a shared buffer
	// (when the wire message is just a reference to it.)
	LongMessageAllocation *lma;
};


