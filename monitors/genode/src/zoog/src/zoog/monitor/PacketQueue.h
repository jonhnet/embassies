#pragma once

#include <base/semaphore.h>
#include <base/lock.h>
#include <nic_session/connection.h>
#include <net/arp.h>
#include <net/dhcp.h>
#include <net/ethernet.h>
#include <net/ipv4.h>

#include "linked_list.h"

namespace ZoogMonitor {

	struct Packet
	{
		Packet_descriptor descriptor;
		void *payload;
		uint32_t size;
	};

	class Packet_queue
	{
	private:
		Genode::Lock lock;
		LinkedList queue;	/* of Packet */
		Genode::Semaphore semaphore;
	public:
		Packet_queue();
		void insert(Packet *packet);
		Packet *remove();
	};
}
