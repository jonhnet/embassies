#include "ZeroCopyBuf_Memcpy.h"
#include "SendBufferFactory_Memcpy.h"

SendBufferFactory_Memcpy::SendBufferFactory_Memcpy(MallocFactory *mf)
{
	this->mf = mf;
}

ZeroCopyBuf *SendBufferFactory_Memcpy::create_send_buffer(uint32_t len)
{
	return new ZeroCopyBuf_Memcpy(mf, len);
}

