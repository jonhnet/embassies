#include <timer_session/connection.h>
#include "NetworkThread.h"
#include "xax_network_utils.h"
#include "standard_malloc_factory.h"

using namespace Genode;
using namespace Net;
using namespace ZoogMonitor;

Packet_queue::Packet_queue()
{
	linked_list_init(&queue, standard_malloc_factory_init());
}

void Packet_queue::insert(Packet *packet)
{
	lock.lock();
	linked_list_insert_tail(&queue, packet);
//	semaphore.up();
	lock.unlock();
}

Packet *Packet_queue::remove()
{
//	PDBG("down");
//	semaphore.down();
//	PDBG("done");
	lock.lock();
	Packet *result = (Packet *) linked_list_remove_head(&queue);
	lock.unlock();
	return result;
}

/* based on nic_bridge model */

void NetworkThread::next_packet(void** src, Genode::size_t *size) {
	// NB I don't think we need to lock access to the _nic_session,
	// and we don't *want* to, because then we block on releasing
	// a previous packet through the same interface.
	/* get next packet from NIC driver */
	_rx_packet = _nic_session->rx()->get_packet();
	*src       = _nic_session->rx()->packet_content(_rx_packet);
	*size      = _rx_packet.size();

	_dbg_total += _rx_packet.size();
	if (_dbg_total > _dbg_next_mark)
	{
		PDBG("%d MB", _dbg_total>>20);
		_dbg_next_mark += (1<<20);
	}

#if ACCOUNT
	account_received_packets_accepted+=1;
	PDBG("received packets accepted %d", account_received_packets_accepted);
#endif // ACCOUNT
}

void NetworkThread::send_arp_message(
	uint16_t arp_opcode,
	Ethernet_frame::Mac_address from_mac,
	Ethernet_frame::Mac_address to_mac,
	Ipv4_packet::Ipv4_address from_ip,
	Ipv4_packet::Ipv4_address to_ip)
{
	Nic::Session::Tx::Source *source = _nic_session->tx();
	
	size_t reply_eth_size = sizeof(Ethernet_frame) + sizeof(Arp_packet);
	Packet_descriptor packet = source->alloc_packet(reply_eth_size);

#if ACCOUNT
	account_sent_packets_allocated += 1;
	PDBG("account_sent_packets_allocated %d", account_sent_packets_allocated);
#endif // ACCOUNT

	Ethernet_frame *reply_eth = new (source->packet_content(packet))
		Ethernet_frame(reply_eth_size);
	reply_eth->src(from_mac);
	reply_eth->dst(to_mac);
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
	reply_arp->opcode(arp_opcode);
	reply_arp->src_mac(from_mac);
	reply_arp->dst_mac(to_mac);
	reply_arp->src_ip(from_ip);
	reply_arp->dst_ip(to_ip);
	PDBG("arp out for %08x", ((uint32_t*) to_ip.addr)[0]);

	_internal_send(packet);
}

bool NetworkThread::handle_arp(Ethernet_frame *eth, size_t size)
{
	// wow, that's a distressing way to use new. Interesting and
	// fairly creepy. I guess I would have done a cast to struct, but then
	// I couldn't pass in arguments to a constructor. Huh.
	// Learn something new every day.
	Arp_packet *arp = new (eth->data())
		Arp_packet(size - sizeof(Ethernet_frame));

	if (!arp->ethernet_ipv4()) {
		return true;	// handled.
	}

	if (arp->opcode() == Arp_packet::REQUEST) {
		PDBG("got arp for low octet %d", arp->dst_ip().addr[3]);

		send_arp_message(
			Arp_packet::REPLY,
			_nic_session->mac_address().addr,
			arp->src_mac(),
			arp->dst_ip(),
			arp->src_ip());
	} else {
		if (arp->src_ip()==xip_to_genode_ipv4(_ifconfig->gateway))
		{
			gateway_mac_address = arp->src_mac();
		}
		PDBG("Recorded gateway mac address");
	}

	return true;
}

void NetworkThread::send_packet(AllocatedBuffer *allocated_buffer)
{
	IPInfo ipinfo;
	decode_ip_packet(&ipinfo, allocated_buffer->payload(), allocated_buffer->capacity());
	if (!ipinfo.packet_valid) {
		PWRN("Dropping invalid packet");
		return;
	}
	// TODO this isn't yet race-secure: we should be dropping the sharedness
	// of the dataspace (is it revocable?), or at least reasoning more
	// carefully about race-resistance.
	size_t reply_eth_size = sizeof(Ethernet_frame) + ipinfo.packet_length;
	Packet_descriptor packet = _nic_session->tx()->alloc_packet(reply_eth_size);

#if ACCOUNT
	account_sent_packets_allocated += 1;
	PDBG("account_sent_packets_allocated %d", account_sent_packets_allocated);
#endif // ACCOUNT

	Ethernet_frame *reply_eth = new (_nic_session->tx()->packet_content(packet))
		Ethernet_frame(reply_eth_size);
	reply_eth->src(_nic_session->mac_address().addr);
	reply_eth->dst(gateway_mac_address);
	reply_eth->type(Ethernet_frame::IPV4);
	memcpy(&reply_eth[1], allocated_buffer->payload(), ipinfo.packet_length);
//	PDBG("Wrote a packet of IP size %d", ipinfo.packet_length);
	_internal_send(packet);
}

void NetworkThread::_internal_send(Packet_descriptor packet)
{
	Nic::Session::Tx::Source *source = _nic_session->tx();
	// if we don't ack packets before allocating more, the sink will
	// fill our ack queue, and we'll get the dreaded
	// "invalid signal context capability". (TODO We'll also get that
	// if we send too fast and fill the submit queue, so we'll need
	// to eventually figure out the signal handler thing.)
	while (source->ack_avail()) {
		//PDBG("releasing a packet.");
		Packet_descriptor ack_packet = source->get_acked_packet();
		source->release_packet(ack_packet);
	}
	while (!source->ready_to_submit())
	{
		PDBG("Huh, not ready to submit. What shall we do? Stalling synchronously.");
		timer->msleep(200);
	}
#if ACCOUNT
	account_sent_packets_submitted += 1;
	PDBG("account_sent_packets_submitted %d", account_sent_packets_submitted);
#endif // ACCOUNT
	source->submit_packet(packet);
}

Net::Ipv4_packet::Ipv4_address NetworkThread::xip_to_genode_ipv4(XIPAddr xipaddr)
{
	return Net::Ipv4_packet::Ipv4_address((void*) &xipaddr.xip_addr_un.xv4_addr);
}

void NetworkThread::dispatch_debug_request(char *dbg_request_str)
{
	const char *args;
	for (char *separator = dbg_request_str; ; separator++)
	{
		if (separator[0]=='\0')
		{
			args = "";
			break;
		}
		if (separator[0]==' ')
		{
			separator[0] = '\0';
			args = separator + 1;
			break;
		}
	}

	if (strcmp(dbg_request_str, "packets")==0)
	{
		bool state = strcmp(args, "true")==0;
		PDBG("set packet debugging to %d", state);
		_dbg_flags->packets = state;
	}
	else if (strcmp(dbg_request_str, "core")==0)
	{
		PDBG("Invoking pal core dump");
		_core_dump_invoke_signal_transmitter->submit();
	}
	else
	{
		PDBG("Unknown debug request '%s' args '%s'", dbg_request_str, args);
	}
}

void NetworkThread::handle_ipv4(Ethernet_frame *eth, size_t eth_sz)
{
	lock.lock();

	uint8_t *ip_packet = (uint8_t*) eth->data();
	IPInfo info;
	decode_ip_packet(&info, ip_packet, eth_sz - sizeof(Ethernet_frame));
	if (info.packet_valid
		&& info.layer_4_protocol==PROTOCOL_UDP
		&& info.fragment_offset==0
		&& !info.more_fragments
		&& info.payload_length >= sizeof(UDPPacket)
		&& Z_NTOHG(((UDPPacket *) (ip_packet+info.payload_offset))->dest_port)
			== DEBUG_PORT)
	{
		char *dbg_request = (char*)
			(ip_packet+info.payload_offset+sizeof(UDPPacket));
		int udp_payload_length = info.payload_length - sizeof(UDPPacket);
		MallocFactory *mf = standard_malloc_factory_init();
		char *dbg_request_str = (char*) mf_malloc(mf, udp_payload_length + 1);
		memcpy(dbg_request_str, dbg_request, udp_payload_length);
		dbg_request_str[udp_payload_length] = '\0';
		dispatch_debug_request(dbg_request_str);
		mf_free(mf, dbg_request_str);
	}
	else
	{
		Packet *packet = new Packet;
		packet->descriptor = _rx_packet;
		packet->payload = eth->data();
		packet->size = eth_sz - sizeof(Ethernet_frame);
		packet_queue.insert(packet);
		_rx_packet = Packet_descriptor();	 // don't acknowledge this one yet, then
		// Tell client that a packet's ready
		_network_receive_signal_transmitter->submit();
	}
	lock.unlock();
}

void NetworkThread::entry()
{
	void *src;
	size_t eth_sz;

	// Genode forgot to include Ipv4_packet::BROADCAST in its library builds.
	const Net::Ipv4_packet::Ipv4_address _IPV4_BROADCAST(0xFF);

	lock.lock();
	send_arp_message(
		Arp_packet::REQUEST,
		_nic_session->mac_address().addr,
		Ethernet_frame::BROADCAST,
		xip_to_genode_ipv4(_ifconfig->local_interface),
		xip_to_genode_ipv4(_ifconfig->gateway));
	lock.unlock();

	while (true)
	{
		try {
			acknowledge_last_packet();
			next_packet(&src, &eth_sz);

			/* parse ethernet frame header */
			Ethernet_frame *eth = new (src) Ethernet_frame(eth_sz);
			switch (eth->type()) {
			case Ethernet_frame::ARP:
				{
					PDBG("ARP frame");
					if (!handle_arp(eth, eth_sz))
						continue;
					break;
				}
			case Ethernet_frame::IPV4:
				{
					//PDBG("IPv4 frame");
					handle_ipv4(eth, eth_sz);
					break;
				}
			default:
				;
			}

//			/* broadcast packet ? */
//			broadcast_to_clients(eth, eth_sz);

//			finalize_packet(eth, eth_sz);
		} catch(Arp_packet::No_arp_packet) {
			PWRN("Invalid ARP packet!");
		} catch(Ethernet_frame::No_ethernet_frame) {
			PWRN("Invalid ethernet frame");
		} catch(Dhcp_packet::No_dhcp_packet) {
			PWRN("Invalid IPv4 packet!");
		} catch(Ipv4_packet::No_ip_packet) {
			PWRN("Invalid IPv4 packet!");
		} catch(Udp_packet::No_udp_packet) {
			PWRN("Invalid UDP packet!");
		}
	}
}

NetworkThread::NetworkThread(
		Nic::Connection *_nic_session,
		XIPifconfig *ifconfig,
		Signal_transmitter *network_receive_signal_transmitter,
		Signal_transmitter *core_dump_invoke_signal_transmitter,
		DebugFlags *dbg_flags,
		Timer::Connection *timer)
	: _nic_session(_nic_session),
	  _ifconfig(ifconfig),
	  _network_receive_signal_transmitter(network_receive_signal_transmitter),
	  _core_dump_invoke_signal_transmitter(core_dump_invoke_signal_transmitter),
	  _dbg_flags(dbg_flags),
	  timer(timer),
	  _dbg_total(0),
	  _dbg_next_mark(0),
	  account_received_packets_accepted(0),
	  account_received_packets_acknowledged(0),
	  account_sent_packets_allocated(0),
	  account_sent_packets_submitted(0)
{
#if 0
	enum { PAYLOAD_SIZE = 2000 };
	int i;
	for (i=0; i<100; i++) {
		PDBG("A %x", i);
		Nic::Session::Tx::Source *source = _nic_session->tx();
		size_t reply_eth_size = sizeof(Ethernet_frame) + PAYLOAD_SIZE;
		Packet_descriptor packet = source->alloc_packet(reply_eth_size);
		Ethernet_frame *reply_eth = new (source->packet_content(packet))
			Ethernet_frame(reply_eth_size);
		reply_eth->src(_nic_session->mac_address().addr);
//		reply_eth->dst(_nic_session->mac_address().addr);
		reply_eth->type(Ethernet_frame::ARP);
		memset(&reply_eth[1], i, PAYLOAD_SIZE);
		PDBG("B");
		source->submit_packet(packet);
		PDBG("C");
		timer.msleep(500);
		PDBG("D");
	}
#endif
}

Packet *NetworkThread::receive()
{
	return packet_queue.remove();
}

void NetworkThread::acknowledge_last_packet()
{
	lock.lock();
	if(_rx_packet.valid()) {
#if ACCOUNT
		account_received_packets_acknowledged+=1;
		PDBG("received packets acknowledged %d (ack_last)", account_received_packets_acknowledged);
#endif // ACCOUNT
		_nic_session->rx()->acknowledge_packet(_rx_packet);
	}
	lock.unlock();
}

void NetworkThread::release(Packet *packet)
{
	lock.lock();
	if (packet->descriptor.valid()) {
#if ACCOUNT
		account_received_packets_acknowledged+=1;
		PDBG("received packets acknowledged %d (release)", account_received_packets_acknowledged);
#endif // ACCOUNT
		_nic_session->rx()->acknowledge_packet(packet->descriptor);
	}
	delete packet;
	lock.unlock();
}
