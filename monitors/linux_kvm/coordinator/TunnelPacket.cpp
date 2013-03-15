#include <string.h>
#include <malloc.h>
#include "TunnelPacket.h"

TunnelPacket::TunnelPacket(void *data, uint32_t size)
{
	this->data = malloc(size);
	lite_assert(this->data!=NULL);
	memcpy(this->data, data, size);
	_construct(this->data, size);
}

TunnelPacket::~TunnelPacket()
{
	free(data);
}
