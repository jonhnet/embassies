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

#include "pal_abi/pal_abi.h"
#include "xax_network_defs.h"
#include "packetfilter.h"
#include "malloc_factory.h"
#include "XStreamXdt.h"
#include "SyncFactory_TimedZutex.h"

// C++-only interfaces
#ifdef __cplusplus

#include "ZeroCopyBuf.h"
#include "ZClock.h"
#include "FragmentReassembly.h"
#include "ZLCEmit.h"
#include "XSNReceiveAccounting.h"

class XaxSkinnyNetwork : public PacketDispatcherIfc {
public:
	XaxSkinnyNetwork(ZoogDispatchTable_v1 *zdt, MallocFactory *mf);
	void enable_random_drops(bool enable);

	ZClock *get_clock();
		// Sorry, clock owned by network. Weird. Both one-off resources.

	SyncFactory *get_timer_sf();
		// Get a SyncFactory that implements timed event wait. That gets
		// built here, because it depends on ZClock.

	void free_packet_filter(PFHandler *ph);
	uint16_t allocate_port_number();
	PFHandler *network_register_handler(uint8_t ipver, uint8_t ip_protocol, uint16_t udp_port);
	void unregister(PFHandler *ph);
	ZoogDispatchTable_v1 *get_xdt();
	MallocFactory *get_malloc_factory();
	XIPifconfig *get_ifconfig(XIPVer ipver);

	void dispatch_packet(ZeroCopyBuf *zcb, IPInfo *info);

private:
	ZoogDispatchTable_v1 *zdt;
	MallocFactory *mf;
	ZLCEmit *ze;

	SyncFactory_Zutex _simple_sf;
	SyncFactoryMutex *_mutex;
	ZClock _zclock;
	SyncFactory_TimedZutex _timer_sf;
	FragmentReassembly fragment_reassembly;

	ZoogHostAlarms *alarms;
	PFTable pftable;
	uint16_t next_port;
	PFHandler *clock_sentinel;
	XStreamXdt xstream;	// used as a debug output channel
	XIPifconfig ifconfigs[10];
	int num_ifconfigs;

	static void _network_dispatcher_thread_s(void *v_xsn);
	void _network_dispatcher_thread();
	ZoogNetBuffer *_blocking_receive();

	struct s_placebo {
		XaxSkinnyNetworkPlacebo placebo;
		XaxSkinnyNetwork *x_this;
	} placebo;
	static XaxSkinnyNetworkPlacebo_Methods placebo_methods;
	static ZoogDispatchTable_v1 *get_xdt(XaxSkinnyNetworkPlacebo *xsnp);
	static MallocFactory *get_malloc_factory(XaxSkinnyNetworkPlacebo *xsnp);
	void _discard_packet(ZoogNetBuffer *znb);
	bool test_random_drops();

	bool _enable_random_drops;
	int _random_drop_state;
	XSNReceiveAccounting _receive_accounting;
};

void udp_send_packet(XaxSkinnyNetwork *xsn,
	UDPEndpoint source_ep, UDPEndpoint dest_ep, char *data, int data_length);

PFHandler *xsn_network_register_handler(XaxSkinnyNetwork *xsn, uint8_t ip_protocol, uint16_t udp_port);
PFHandler *xsn_network_add_default_handler(XaxSkinnyNetwork *xsn);

uint16_t xsn_allocate_port_number(XaxSkinnyNetwork *xsn);

typedef struct {
	XaxSkinnyNetwork *xsn;
	bool valid;
	uint16_t local_port_host_order;
	UDPEndpoint local_ep;
	PFHandler *ph;
} XaxSkinnySocket;

bool xax_skinny_socket_init(XaxSkinnySocket *skinny, XaxSkinnyNetwork *xsn, uint16_t port_host_order, XIPVer ipver);
	// returns false if no IP address is available matching ipver
void xax_skinny_socket_free(XaxSkinnySocket *skinny);

ZoogNetBuffer *xax_skinny_socket_prepare_packet(XaxSkinnySocket *skinny, UDPEndpoint dest_ep, int payload_size, void **out_payload);
void xax_skinny_socket_send(XaxSkinnySocket *skinny, ZoogNetBuffer *znb, bool release);

int xax_skinny_socket_recv_buf(XaxSkinnySocket *skinny, uint8_t *buf, uint32_t data_len, UDPEndpoint *sender_ep);

ZeroCopyBuf *xax_skinny_socket_recv_packet(XaxSkinnySocket *skinny, UDPEndpoint *sender_ep);

#endif // __cplusplus

