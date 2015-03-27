#pragma once

#include <base/signal.h>
#include "pal_abi/pal_types.h"
#include "pal_abi/pal_net.h"
#include "PacketQueue.h"
#include "AllocatedBuffer.h"

namespace ZoogMonitor {
	class NetworkThread;

	class NetworkTerminus
	{
	private:
		uint8_t _identifier;
		XIPifconfig _ifconfig;
		Signal_transmitter *_network_receive_signal_transmitter;
		Signal_transmitter *_core_dump_invoke_signal_transmitter;
		Packet_queue packet_queue;
		NetworkThread* _network_thread;

	public:
		NetworkTerminus(
			uint8_t identifier,
			XIPifconfig* ifconfig,
			Signal_transmitter *_network_receive_signal_transmitter,
			Signal_transmitter *_core_dump_invoke_signal_transmitter,
			NetworkThread* network_thread);
			// real object
		NetworkTerminus(uint8_t identifier);
			// hash key

		uint8_t get_identifier();
		void signal_core_dump();
		void enqueue_packet(Packet* packet);
		void notify_client();
		Packet *receive();
		void release(Packet *packet);
		void send_packet(AllocatedBuffer *allocated_buffer);

		static uint32_t hash(const void* v_a);
		static int cmp(const void* v_a, const void* v_b);
	};
}
