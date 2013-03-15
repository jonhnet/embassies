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

#include "SocketFactory.h"

class SocketFactory_Counter; 

class CounterSocket : public AbstractSocket {
private:
	AbstractSocket *us;
	SocketFactory_Counter *sfc;
public:
	CounterSocket(AbstractSocket* us, SocketFactory_Counter *sfc);
	virtual ~CounterSocket();

	virtual ZeroCopyBuf *zc_allocate(uint32_t payload_len);
	virtual bool zc_send(UDPEndpoint *remote, ZeroCopyBuf *zcb);
	virtual void zc_release(ZeroCopyBuf *zcb);

	virtual ZeroCopyBuf *recvfrom(UDPEndpoint *out_remote);
};

class SocketFactory_Counter : public SocketFactory {
private:
	SocketFactory* usf;
	uint32_t sent_bytes, received_bytes;
public:
	SocketFactory_Counter(SocketFactory* usf);
	virtual AbstractSocket *new_socket(UDPEndpoint *local, bool want_timeouts);
	void _count(int _sent_bytes);
};

