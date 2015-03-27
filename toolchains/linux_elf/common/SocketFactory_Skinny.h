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
#include "xax_skinny_network.h"

class SkinnySocket : public AbstractSocket {
public:
	~SkinnySocket();

	ZeroCopyBuf *zc_allocate(uint32_t payload_len);
	bool zc_send(UDPEndpoint *remote, ZeroCopyBuf *zcb, uint32_t final_len);
		// Does not release the zcb, so you can send it again and again
		// if you like.
	void zc_release(ZeroCopyBuf *zcb);

	bool sendto(UDPEndpoint *remote, void *buf, uint32_t buf_len);

	ZeroCopyBuf *recvfrom(UDPEndpoint *out_remote);

private:
	friend class SocketFactory_Skinny;
	SkinnySocket(XaxSkinnyNetwork *xsn, UDPEndpoint *local, XIPVer ipver, bool want_timeouts);
	bool _internal_send(UDPEndpoint *remote, ZeroCopyBuf *zcb);

	XaxSkinnyNetwork *xsn;
	XaxSkinnySocket skinny_socket;
	bool want_timeouts;
};

class SocketFactory_Skinny : public SocketFactory {
public:
	SocketFactory_Skinny(XaxSkinnyNetwork *xsn);
	AbstractSocket *new_socket(UDPEndpoint* local, UDPEndpoint* remote, bool want_timeouts);

private:
	XaxSkinnyNetwork *xsn;
};
