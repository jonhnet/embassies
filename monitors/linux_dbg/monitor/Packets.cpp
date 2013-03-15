#include <assert.h>
#include <malloc.h>
#include <string.h>
#include "Packets.h"

PacketIfc::PacketIfc(OriginType origin_type, const char *dbg_purpose)
	: origin_type(origin_type),
	  _dbg_purpose(dbg_purpose)
{
}

//////////////////////////////////////////////////////////////////////////////

void count_memcpy(uint32_t len)
{
	static uint32_t counter = 0;
	static uint32_t gran = 0;
	counter += len;
	uint32_t t_gran = counter>>(20+4);
	if (t_gran > gran)
	{
		fprintf(stderr, "memcpy %dMB\n", t_gran<<4);
		gran = t_gran;
	}
}

SmallPacket::SmallPacket(const char *data, uint32_t len, OriginType origin_type, const char *dbg_purpose)
	: PacketIfc(origin_type, dbg_purpose)
{
	_data = (char*) malloc(len);
	memcpy(_data, data, len);
	count_memcpy(len);
	_len = len;
}

SmallPacket::~SmallPacket()
{
	free(_data);
}

uint32_t SmallPacket::len()
{
	return _len;
}

char *SmallPacket::data()
{
	return _data;
}

PacketIfc *SmallPacket::copy(const char *dbg_purpose)
{
	return new SmallPacket(_data, _len, get_origin_type(), dbg_purpose);
}

//////////////////////////////////////////////////////////////////////////////

XPBPacket::XPBPacket(XPBPoolElt *xpbe, OriginType origin_type)
	: PacketIfc(origin_type, "unset")
{
	this->_xpbe = xpbe;
	_xpbe->freeze();
}

XPBPacket::~XPBPacket()
{
	_xpbe->unfreeze();
}

uint32_t XPBPacket::len()
{
	return _xpbe->xpb->capacity;
}

char *XPBPacket::data()
{
	return (char*) _xpbe->xpb->un.znb._data;
}

PacketIfc *XPBPacket::copy(const char *dbg_purpose)
{
	assert(false);		// unimpl
	return NULL;
}

