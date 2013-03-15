#include "MonitorPacket.h"

MonitorPacket::MonitorPacket(Message *m)
{
	this->m = m;

	dp = (CMDeliverPacket *) m->get_payload();
	lite_assert(dp->hdr.opcode == co_deliver_packet);
	_construct(dp->payload, m->get_payload_size() - sizeof(*dp));
}

MonitorPacket::~MonitorPacket()
{
	delete m;
}
