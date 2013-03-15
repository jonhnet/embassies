#include "LongPacket.h"
#include "KvmNetBufferContainer.h"

LongPacket::LongPacket(CMDeliverPacketLong*, LongMessageAllocation* lma)
{
#if 0
	decode_ip_packet(&ipinfo, dpl->prefix, sizeof(dpl->prefix));
	SAFETY_CHECK(ipinfo.packet_valid);
	SAFETY_CHECK(ipinfo.packet_length == size);
	this->size = ipinfo.packet_length;
	this->payload = 0x11111111;
		// TODO expedient sentinel: mark as "valid" without
		// actually exposing the payload, since we don't necessarily
		// have it mapped. (Actually, we do, but we can fix that eventually.)
	this->lma = lma;
#endif
	KvmNetBufferContainer* knbc = (KvmNetBufferContainer*) lma->map();
	_construct(ZNB_DATA(knbc->get_znb()), lma->get_mapped_size() - knbc->padding);

	this->lma = lma;
	lma->add_ref("LongPacket");
}

LongPacket::~LongPacket()
{
	lma->drop_ref("~LongPacket");
	lma = NULL;
}
