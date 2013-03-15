#include <string.h>
#include <malloc.h>
#include "Packet.h"

Packet::Packet(uint32_t cm_size, CMDeliverPacket *dp)
	: size(cm_size - sizeof(CMDeliverPacket)),
	  data((uint8_t*) malloc(cm_size)),
	  lma(NULL)
{
	memcpy(data, dp->payload, size);
}

Packet::Packet(LongMessageAllocation* lma)
	: size(0),
	  data(NULL),
	  lma(lma)
{
}

Packet::~Packet()
{
	free(data);
}


uint32_t Packet::get_size()
{
	lite_assert(data!=NULL);	// I don't think this is used on the lma
		// path; else fix size(0) in ctor!
	return size;
}

uint8_t *Packet::get_data()
{
	return data;
}

LongMessageAllocation* Packet::get_lma()
{
	return lma;
}

uint32_t Packet::get_dbg_size()
{
	if (lma)
	{
		// only approximate, hence 'dbg_'. Too lazy to parse the ip
		// header inside.
		return lma->get_mapped_size();
	}
	else
	{
		return size;
	}
}
