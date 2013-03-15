#include "ZutexHandle.h"

ZASMethods3 ZutexHandle::_zutex_handle_zas_method_table =
	{ NULL, ZutexHandle::_zas_check_s, NULL, NULL };

ZutexHandle::ZutexHandle(uint32_t *zutex)
{
	this->zutex_value = -1;
	this->zutex = zutex;
	pp.zas.method_table = &_zutex_handle_zas_method_table;
	pp.self = this;
}

ZAS *ZutexHandle::get_zas(ZASChannel channel)
{
	return &pp.zas;
}

bool ZutexHandle::_zas_check_s(ZoogDispatchTable_v1 *zdt, ZAS *zas, ZutexWaitSpec *spec)
{
	struct s_pointer_package *pp = (struct s_pointer_package *) zas;
	return pp->self->_zas_check(spec);
}

bool ZutexHandle::_zas_check(ZutexWaitSpec *spec)
{
	if (*zutex != zutex_value)
	{
		return true;
	}
	spec->zutex = zutex;
	spec->match_val = zutex_value;
	return false;
}

uint32_t ZutexHandle::read(XfsErr *err, void *dst, size_t len)
{
	zutex_value = *zutex;
	*err = XFS_NO_ERROR;
	return len;
}
