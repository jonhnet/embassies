#include "LiteLib.h"
#include "ZeroCopyBuf_Memcpy.h"

ZeroCopyBuf_Memcpy::ZeroCopyBuf_Memcpy(MallocFactory *mf, uint32_t len)
{
	this->_buf = (uint8_t*) mf_malloc(mf, len);
	this->_len = len;
	this->mf = mf;
}

ZeroCopyBuf_Memcpy::ZeroCopyBuf_Memcpy(MallocFactory *mf, uint8_t *data, uint32_t len)
{
	this->_buf = (uint8_t*) mf_malloc(mf, len);
	lite_memcpy(this->_buf, data, len);
	this->_len = len;
	this->mf = mf;
}

ZeroCopyBuf_Memcpy::~ZeroCopyBuf_Memcpy()
{
	mf_free(mf, _buf);
}

uint8_t *ZeroCopyBuf_Memcpy::data()
{
	return _buf;
}

uint32_t ZeroCopyBuf_Memcpy::len()
{
	return _len;
}

