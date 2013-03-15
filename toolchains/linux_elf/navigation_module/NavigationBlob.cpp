#include <malloc.h>
#include <string.h>

#include "NavigationBlob.h"
#include "LiteLib.h"

NavigationBlob::NavigationBlob()
{
	init(1024);
}

NavigationBlob::NavigationBlob(uint32_t start_capacity)
{
	if (start_capacity==0)
	{
		start_capacity = 1;	// might be unpacking an empty blob.
	}
	init(start_capacity);
}

NavigationBlob::NavigationBlob(uint8_t *bytes, uint32_t size)
{
	init(size);
	memcpy(_buf, bytes, size);
//	dbg_check_magic();
	_size = size;
}

#if 0
void NavigationBlob::dbg_check_magic()
{
	lite_assert(_buf[_capacity] == DBG_MAGIC);
}
#endif

void NavigationBlob::init(uint32_t start_capacity)
{
	_capacity = start_capacity;
	_buf = (uint8_t*) malloc(_capacity /*+1*/ );
//	_buf[_capacity] = DBG_MAGIC;
	_dbg_buf = _buf;
	_size = 0;
	_read_ptr = 0;
}

NavigationBlob::~NavigationBlob()
{
	free(_buf);
	_buf = NULL;
}

void NavigationBlob::grow(uint32_t min_capacity)
{
	uint32_t new_capacity = _capacity;
	while (new_capacity < min_capacity)
	{
		new_capacity *= 2;
	}
	lite_assert(new_capacity > _capacity);
	uint8_t* new_buf = (uint8_t*) malloc(new_capacity /*+1*/ );
//	new_buf[new_capacity] = DBG_MAGIC;
	_dbg_buf = new_buf;
	lite_assert(_size < new_capacity);
	memcpy(new_buf, _buf, _size);
	free(_buf);
	_buf = new_buf;
//	dbg_check_magic();
	_capacity = new_capacity;
}

void NavigationBlob::append(void *data, uint32_t size)
{
	if (_size + size > _capacity)
	{
		grow(_size + size);
	}
	memcpy(_buf+_size, data, size);
//	dbg_check_magic();
	_size += size;
}

void NavigationBlob::append(NavigationBlob *blob)
{
	uint32_t dbg_blob_token = DBG_BLOB_TOKEN;
	append(&dbg_blob_token, sizeof(dbg_blob_token));
	append(&blob->_size, sizeof(blob->_size));
	append(blob->_buf, blob->_size);
}

void NavigationBlob::read(void *data, uint32_t size)
{
	lite_assert(_read_ptr + size <= _size);
	memcpy(data, _buf+_read_ptr, size);
	_read_ptr += size;
}

uint8_t *NavigationBlob::read_in_place(uint32_t size)
{
	lite_assert(_read_ptr + size <= _size);
	uint8_t *result = _buf+_read_ptr;
	_read_ptr += size;
	return result;
}

uint8_t *NavigationBlob::write_in_place(uint32_t size)
{
	if (_size + size > _capacity)
	{
		grow(_size + size);
	}
	uint8_t *result = _buf + _size;
	_size += size;
	return result;
}

NavigationBlob *NavigationBlob::read_blob()
{
	uint32_t dbg_blob_token = 0;
	read(&dbg_blob_token, sizeof(dbg_blob_token));
	lite_assert(dbg_blob_token == DBG_BLOB_TOKEN);
	uint32_t size;
	read(&size, sizeof(size));
	NavigationBlob *out = new NavigationBlob(size);
	read(out->_buf, size);
	out->_size = size;
	return out;
}

uint32_t NavigationBlob::size()
{
	return _size - _read_ptr;
}

