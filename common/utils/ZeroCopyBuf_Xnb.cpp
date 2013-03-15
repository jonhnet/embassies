#include "LiteLib.h"
#include "ZeroCopyBuf_Xnb.h"

ZeroCopyBuf_Xnb::ZeroCopyBuf_Xnb(ZoogDispatchTable_v1 *xdt, uint32_t len)
{
	this->xdt = xdt;
	this->xnb = (xdt->zoog_alloc_net_buffer)(len);
	this->_offset = 0;
	this->_len = len;
}

ZeroCopyBuf_Xnb::ZeroCopyBuf_Xnb(ZoogDispatchTable_v1 *xdt, ZoogNetBuffer *xnb, uint32_t offset, uint32_t len)
{
	this->xdt = xdt;
	this->xnb = xnb;
	this->_offset = offset;
	this->_len = len;
	lite_assert(len <= xnb->capacity-offset);
}

ZeroCopyBuf_Xnb::~ZeroCopyBuf_Xnb()
{
	if (xnb!=NULL)
	{
		(xdt->zoog_free_net_buffer)(xnb);
	}
}

uint8_t *ZeroCopyBuf_Xnb::data()
{
	return ((uint8_t*)ZNB_DATA(xnb))+_offset;
}

uint32_t ZeroCopyBuf_Xnb::len()
{
	return _len;
}

/*
ZoogNetBuffer *ZeroCopyBuf_Xnb::claim_xnb()
{
	ZoogNetBuffer *result = xnb;
	this->xnb = NULL;
	return result;
}
*/

ZoogNetBuffer *ZeroCopyBuf_Xnb::peek_xnb()
{
	return xnb;
}
