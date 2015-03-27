#include "NetworkTerminus.h"
#include "NetworkThread.h"

namespace ZoogMonitor {

NetworkTerminus::NetworkTerminus(
	uint8_t identifier,
	XIPifconfig* ifconfig,
	Signal_transmitter *_network_receive_signal_transmitter,
	Signal_transmitter *_core_dump_invoke_signal_transmitter,
	NetworkThread* network_thread)
	: _identifier(identifier),
	  _ifconfig(*ifconfig),
	  _network_receive_signal_transmitter(_network_receive_signal_transmitter),
	  _core_dump_invoke_signal_transmitter(_core_dump_invoke_signal_transmitter),
	  _network_thread(network_thread)
{
	_network_thread->register_terminus(this);
}

NetworkTerminus::NetworkTerminus(
	uint8_t identifier)
	: _identifier(identifier)
{
}

uint8_t NetworkTerminus::get_identifier()
{
	return _identifier;
}

void NetworkTerminus::signal_core_dump()
{
	PDBG("NetworkTerminus::signal_core_dump: _core_dump_invoke_signal_transmitter = 0x%08x\n", (int) _core_dump_invoke_signal_transmitter);
	_core_dump_invoke_signal_transmitter->submit();
}

void NetworkTerminus::enqueue_packet(Packet* packet)
{
	packet_queue.insert(packet);
}

void NetworkTerminus::notify_client()
{
	_network_receive_signal_transmitter->submit();
}

uint32_t NetworkTerminus::hash(const void* v_a)
{
	NetworkTerminus* a = (NetworkTerminus*) v_a;
	return a->_identifier;
}

int NetworkTerminus::cmp(const void* v_a, const void* v_b)
{
	NetworkTerminus* a = (NetworkTerminus*) v_a;
	NetworkTerminus* b = (NetworkTerminus*) v_b;
	return a->_identifier - b->_identifier;
}

Packet *NetworkTerminus::receive()
{
	return packet_queue.remove();
}

void NetworkTerminus::release(Packet *packet)
{
	_network_thread->release(packet);
}

void NetworkTerminus::send_packet(AllocatedBuffer *allocated_buffer)
{
	_network_thread->send_packet(allocated_buffer);
}

}
