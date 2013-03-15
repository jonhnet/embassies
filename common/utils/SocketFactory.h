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

#include "pal_abi/pal_net.h"
#include "xax_network_utils.h"
#include "ZeroCopyBuf.h"

class AbstractSocket {
public:
	virtual ~AbstractSocket() {}

	virtual ZeroCopyBuf *zc_allocate(uint32_t payload_len) = 0;
	virtual bool zc_send(UDPEndpoint *remote, ZeroCopyBuf *zcb) = 0;
	virtual void zc_release(ZeroCopyBuf *zcb) = 0;

	bool sendto(UDPEndpoint *remote, void *buf, uint32_t buf_len);
		// memcpy-rich glue

	virtual ZeroCopyBuf *recvfrom(UDPEndpoint *out_remote) = 0;
};

class SocketFactory {
public:
	virtual AbstractSocket *new_socket(UDPEndpoint *local, bool want_timeouts) = 0;
		// local == NULL => bind to any port on the local interface;
		// usable only for outgoing connections.
		// want_timeouts: cheese. if you say 'true', and your recv sits for
		// a second or so, you'll get a NULL return. Ugh. Should have a better
		// way, with zutexes.

		// Returns NULL if socket can't be bound. (Yeah, sorry, no error
		// message.) Probably no ipv6 address available, 'be my guess.

	static UDPEndpoint *get_inaddr_any(XIPVer ipver);

private:
	static bool any_init;
	static UDPEndpoint any[2];
};
