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

#include <base/thread.h>
#include <base/signal.h>
#include <base/lock.h>
#include <nic_session/connection.h>
#include <net/arp.h>
#include <net/dhcp.h>
#include <net/ethernet.h>
#include <net/ipv4.h>
#include <timer_session/connection.h>

#include "AllocatedBuffer.h"
#include "DebugFlags.h"
#include "PacketQueue.h"
#include "hash_table.h"
#include "NetworkTerminus.h"

namespace ZoogMonitor {
	class NetworkThread : public Genode::Thread<8192>
	{
	private:
		Nic::Connection *_nic_session;
		XIPifconfig *_prototype_ifconfig;
		Packet_descriptor _rx_packet; /* actual processed packet */
		Net::Ethernet_frame::Mac_address gateway_mac_address;
		Lock lock;
		DebugFlags *_dbg_flags;
		Timer::Connection *timer;
		int _dbg_total;
		int _dbg_next_mark;
		HashTable terminii_table;

		uint32_t account_received_packets_accepted;
		uint32_t account_received_packets_acknowledged;
		uint32_t account_sent_packets_allocated;
		uint32_t account_sent_packets_submitted;

		void entry();

		void acknowledge_last_packet();

		void next_packet(void** src, Genode::size_t *size);

		bool handle_arp(Net::Ethernet_frame *eth, Genode::size_t size);

		enum { DEBUG_PORT = 1111 };
		void dispatch_debug_request(char *dbg_request_str);
		void handle_ipv4(Net::Ethernet_frame *eth, Genode::size_t size);

		void _internal_send(Packet_descriptor packet);

		Net::Ipv4_packet::Ipv4_address xip_to_genode_ipv4(XIPAddr xipaddr);

		void send_arp_message(
			uint16_t arp_opcode,
			Net::Ethernet_frame::Mac_address from_mac,
			Net::Ethernet_frame::Mac_address to_mac,
			Net::Ipv4_packet::Ipv4_address from_ip,
			Net::Ipv4_packet::Ipv4_address to_ip);

		NetworkTerminus* findTerminus(uint32_t identifier);
		static void list_one(void* user, void* a);
		void listTerminii();

	public:
		NetworkThread(
			Nic::Connection *_nic_session,
			XIPifconfig *prototype_ifconfig,
			DebugFlags *dbg_flags,
			Timer::Connection *timer);

		void register_terminus(NetworkTerminus* terminus);
		void send_packet(AllocatedBuffer *allocated_buffer);
		void release(Packet *packet);
	};

}
