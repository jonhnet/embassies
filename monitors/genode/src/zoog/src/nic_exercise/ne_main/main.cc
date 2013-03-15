#include <base/printf.h>
#include <base/env.h>
#include <base/sleep.h>
#include <cap_session/connection.h>
#include <root/component.h>
#include <base/rpc_server.h>
#include <rom_session/connection.h>
#include <timer_session/connection.h>
#include <net/arp.h>
#include <net/dhcp.h>
#include <net/ethernet.h>
#include <net/ipv4.h>

#include <nic_session/connection.h>

using namespace Genode;
using namespace Net;

class T1 : public Thread<4096> {
	private:
		Nic::Connection *_nic;
	public:
		T1(Nic::Connection *nic)
			: _nic(nic) {}
		void entry();
		bool handle_arp(Ethernet_frame *eth, size_t size);
};

bool T1::handle_arp(Ethernet_frame *eth, size_t size) {
	// wow, that's a distressing way to use new. Interesting and
	// fairly creepy. I guess I would have done a cast to struct, but then
	// I couldn't pass in arguments to a constructor. Huh.
	// Learn something new every day.
	Arp_packet *arp = new (eth->data())
		Arp_packet(size - sizeof(Ethernet_frame));

	if (!arp->ethernet_ipv4()) {
		return true;	// handled.
	}

	PDBG("got arp for low octet %d", arp->dst_ip().addr[3]);

	Nic::Session::Tx::Source *source = _nic->tx_channel()->source();
	size_t reply_eth_size = sizeof(Ethernet_frame) + sizeof(Arp_packet);
	Packet_descriptor packet = source->alloc_packet(reply_eth_size);
	Ethernet_frame *reply_eth = new (source->packet_content(packet))
		Ethernet_frame(reply_eth_size);
	reply_eth->src(_nic->mac_address().addr);
	reply_eth->dst(arp->src_mac());
	reply_eth->type(Ethernet_frame::ARP);

	// strangely, Net::Arp_packet interface doesn't provide methods
	// to construct an arp packet from whole cloth; the only use case
	// I see (nic_bridge) builds arp packets by mangling the ones it gets.
	// Rather than patch Arp_packet to add setters, I'll
	// make my own overlay struct right here.
	struct Arp_writeable_overlay {
		Genode::uint16_t _hw_addr_type;
		Genode::uint16_t _prot_addr_type;
		Genode::uint8_t  _hw_addr_sz;
		Genode::uint8_t  _prot_addr_sz;
	};
	
	Arp_writeable_overlay *reply_arp_overlay =
		(Arp_writeable_overlay *) reply_eth->data();
	reply_arp_overlay->_hw_addr_type = bswap((uint16_t) Arp_packet::ETHERNET);
	reply_arp_overlay->_prot_addr_type = bswap((uint16_t) Ethernet_frame::IPV4);
	reply_arp_overlay->_hw_addr_sz = Ethernet_frame::ADDR_LEN;
	reply_arp_overlay->_prot_addr_sz = Ipv4_packet::ADDR_LEN;

	Arp_packet *reply_arp = new (reply_eth->data())
		Arp_packet(sizeof(Arp_packet));
	reply_arp->opcode(Arp_packet::REPLY);
	reply_arp->src_mac(_nic->mac_address().addr);
	reply_arp->dst_mac(arp->src_mac());
	reply_arp->src_ip(arp->dst_ip());
	reply_arp->dst_ip(arp->src_ip());
	source->submit_packet(packet);
	
	return true;
}

void T1::entry() {
	Timer::Connection timer;

	PDBG("nic_exercise/ne_main T1");

	Packet_descriptor _rx_packet;

	PDBG("loop starts");
	while (true)
	{
		if(_rx_packet.valid()) {
			PDBG("ack");
			_nic->rx()->acknowledge_packet(_rx_packet);
		}
		PDBG("get_packet");
		_rx_packet = _nic->rx()->get_packet();

		void *src = _nic->rx()->packet_content(_rx_packet);
		size_t eth_sz = _rx_packet.size();
		Ethernet_frame *eth = new (src) Ethernet_frame(eth_sz);
		switch (eth->type()) {
		case Ethernet_frame::ARP:
			{
				PDBG("ARP frame");
				if (!handle_arp(eth, eth_sz))
					continue;
				break;
			}
		}
	
		//Ethernet_frame *eth = new (src) Ethernet_frame(eth_sz);
		PDBG("Received packet length %d\n", eth_sz);
	}
}

class T2 : public Thread<4096> {
	private:
		Nic::Connection *_nic;
	public:
		T2(Nic::Connection *nic)
			: _nic(nic) {}
		void entry();
};

void T2::entry() {
	Timer::Connection timer;

	//Nic::Session::Tx::Source *source = _nic->tx_channel()->source();
	Nic::Session::Tx::Source *source = _nic->tx();
	while (true)
	{
		while (source->ack_avail()) {
			PDBG("releasing a packet.");
			Packet_descriptor ack_packet = source->get_acked_packet();
			source->release_packet(ack_packet);
		}
		PDBG("ack_avail() == %d", source->ack_avail());
		PDBG("send one");
		size_t reply_eth_size = 770;
		Packet_descriptor packet = source->alloc_packet(reply_eth_size);
		memset(source->packet_content(packet), 17, reply_eth_size);
		source->submit_packet(packet);

		PDBG("sleep one");
		timer.msleep(500);
	}
}

class T3 : public Thread<4096> {
	private:
		Nic::Connection *_nic;
	public:
		T3(Nic::Connection *nic)
			: _nic(nic) {}
		void entry();
};

void T3::entry() {
	Timer::Connection timer;

	while (true)
	{
		Nic::Session::Tx::Source *source = _nic->tx_channel()->source();
		size_t reply_eth_size = 660;
		Packet_descriptor packet = source->alloc_packet(reply_eth_size);
		memset(source->packet_content(packet), 0x66, reply_eth_size);
		source->submit_packet(packet);

		timer.msleep(100);
	}
}
int main(void)
{
	Genode::Allocator_avl _tx_block_alloc(env()->heap());
	Nic::Connection nic(&_tx_block_alloc);
	T1 t1(&nic);
	t1.start();
	T2 t2(&nic);
	t2.start();
//	T3 t3(&nic);
//	t3.start();
	PDBG("ZZZZzzz");
	Genode::sleep_forever();
}
