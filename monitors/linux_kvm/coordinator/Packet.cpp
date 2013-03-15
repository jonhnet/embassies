#include "safety_check.h"
#include "LiteLib.h"
#include "Packet.h"

Packet::Packet()
{
	payload = NULL;
	size = 0;
}

void Packet::_construct(void *payload, uint32_t size)
{
	decode_ip_packet(&ipinfo, payload, size);
	SAFETY_CHECK(ipinfo.packet_valid);
	SAFETY_CHECK(ipinfo.packet_length <= size);
	this->payload = payload;
	this->size = size;
fail:
	return;
}


