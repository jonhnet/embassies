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

#include "xax_network_defs.h"
#include "SocketFactory.h"

class PosixSocket : public AbstractSocket {
public:
	~PosixSocket();

	ZeroCopyBuf *zc_allocate(uint32_t payload_len);
	bool zc_send(UDPEndpoint *remote, ZeroCopyBuf *zcb);
	void zc_release(ZeroCopyBuf *zcb);

	ZeroCopyBuf *allocate_packet(UDPEndpoint *remote, uint32_t payload_len);
	bool send(ZeroCopyBuf *zcb);

	uint32_t get_header_len(uint32_t payload_len);
	bool sendto(UDPEndpoint *remote, ZeroCopyBuf *zcb);
	
	ZeroCopyBuf *recvfrom(UDPEndpoint *out_remote);

private:
	friend class SocketFactory_Posix;
	PosixSocket(UDPEndpoint *local, MallocFactory *mf, bool want_timeouts);

	bool _want_timeouts;
	int sock_fd;
	MallocFactory *mf;
};

class SocketFactory_Posix : public SocketFactory {
public:
	SocketFactory_Posix(MallocFactory *mf);
	AbstractSocket *new_socket(UDPEndpoint *local, bool want_timeouts);
		// local == NULL => listen on any port on the local interface
private:
	MallocFactory *mf;
};
