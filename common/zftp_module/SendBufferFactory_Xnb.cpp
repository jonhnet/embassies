#include "ZeroCopyBuf_Xnb.h"
#include "SendBufferFactory_Xnb.h"

//////////////////////////////////////////////////////////////////////////////

SendBufferFactory_Xnb::SendBufferFactory_Xnb(ZoogDispatchTable_v1 *xdt)
{
	this->xdt = xdt;
}

ZeroCopyBuf *SendBufferFactory_Xnb::create_send_buffer(uint32_t len)
{
	return new ZeroCopyBuf_Xnb(xdt, len);
}
