#include "LiteLib.h"
#include "ZeroCopyView.h"

ZeroCopyView::ZeroCopyView(ZeroCopyBuf *backing, bool own_backing, uint32_t offset, uint32_t len)
{
	this->_backing = backing;
	this->_own_backing = own_backing;
	this->_offset = offset;
	this->_len = len;
	lite_assert(_offset + _len <= backing->len());
}

ZeroCopyView::~ZeroCopyView()
{
	if (_own_backing)
	{
		delete _backing;
	}
}

uint8_t *ZeroCopyView::data()
{
	return _backing->data() + _offset;
}

uint32_t ZeroCopyView::len()
{
	return _len;
}

