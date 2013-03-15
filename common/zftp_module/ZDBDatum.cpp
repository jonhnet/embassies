#include <string.h>
#include <assert.h>
#include <malloc.h>

#include "zftp_util.h"
#include "ZDBDatum.h"

ZDBDatum::ZDBDatum(uint8_t *data, uint32_t len, bool this_owns_data)
{
	memset(&dbt, 0, sizeof(dbt));
	this->dbt.data = data;
	this->dbt.size = len;
	this->this_owns_data = this_owns_data;
	this->marshal_ptr = data;	// for reads
}

ZDBDatum::ZDBDatum(const hash_t *hash)
{
	memset(&dbt, 0, sizeof(dbt));
	this->dbt.data = (uint8_t*) hash;
	this->dbt.size = sizeof(*hash);
	this->this_owns_data = false;
	this->marshal_ptr = (uint8_t*) dbt.data;	// for reads
}

ZDBDatum::ZDBDatum(const char *str)
{
	_init_alloc(sizeof(uint32_t)+strlen(str)+1);
	write_str(str);
}

ZDBDatum::~ZDBDatum()
{
	if (this_owns_data)
	{
		free(dbt.data);
	}
}
	
ZDBDatum::ZDBDatum()
{
	memset(&dbt, 0, sizeof(dbt));
	this->dbt.data = NULL;
	this->dbt.size = 0;
	this->this_owns_data = false;
	this->marshal_ptr = NULL;
}

ZDBDatum *ZDBDatum::alloc()
{
	int alloc_len = marshal_ptr - (uint8_t*)dbt.data;
	return new ZDBDatum(alloc_len);
}

void ZDBDatum::_init_alloc(int len)
{
	memset(&dbt, 0, sizeof(dbt));
	this->dbt.data = malloc(len);
	this->dbt.size = len;
	this->this_owns_data = true;
	this->marshal_ptr = (uint8_t*) dbt.data;	// for writes
}

ZDBDatum::ZDBDatum(int len)
{
	_init_alloc(len);
}

void ZDBDatum::write_buf(const void *buf, int len)
{
	if (dbt.data!=NULL)
	{
		((uint32_t*) marshal_ptr)[0] = len;
	}
	marshal_ptr += sizeof(len);
	if (dbt.data!=NULL)
	{
		memcpy(marshal_ptr, buf, len);
	}
	marshal_ptr += len;
	_assert_no_overrun();
}

void ZDBDatum::write_str(const char *str)
{
	write_buf(str, strlen(str)+1);
}

void ZDBDatum::write_uint32(uint32_t value)
{
	if (dbt.data!=NULL)
	{
		((uint32_t*) marshal_ptr)[0] = value;
	}
	marshal_ptr += sizeof(uint32_t);
	_assert_no_overrun();
}

void ZDBDatum::_assert_no_overrun()
{
	assert(dbt.data==NULL || marshal_ptr <= ((uint8_t*) dbt.data)+dbt.size);
}

hash_t ZDBDatum::read_hash()
{
	hash_t hash;
	memcpy(&hash, marshal_ptr, sizeof(hash));
	marshal_ptr += sizeof(hash);
	_assert_no_overrun();
	return hash;
}

int ZDBDatum::read_buf(void *buf, int capacity)
{
	uint32_t out_len = ((uint32_t*) marshal_ptr)[0];
	assert(out_len <= (uint32_t) capacity);
	marshal_ptr += sizeof(uint32_t);
	memcpy(buf, marshal_ptr, out_len);
	marshal_ptr += out_len;
	_assert_no_overrun();
	return out_len;
}

void ZDBDatum::read_raw(void *buf, int capacity)
{
	memcpy(buf, marshal_ptr, capacity);
	marshal_ptr += capacity;
	_assert_no_overrun();
}

char *ZDBDatum::read_str()
{
	uint32_t obj_len = read_uint32();
	char *ptr = (char*) marshal_ptr;
	marshal_ptr += obj_len;
	_assert_no_overrun();
	assert(ptr[obj_len-1]=='\0');
	return ptr;
}

uint32_t ZDBDatum::read_uint32()
{
	uint32_t value;
	value = ((uint32_t*) marshal_ptr)[0];
	marshal_ptr += sizeof(uint32_t);
	_assert_no_overrun();
	return value;
}

int ZDBDatum::cmp(ZDBDatum *other)
{
	int minsize = min(dbt.size, other->dbt.size);
	int data_diff = memcmp(dbt.data, other->dbt.data, minsize);
	if (data_diff!=0)
	{
		return data_diff;
	}
	// They tie on prefix; compare on length.
	return dbt.size - other->dbt.size;
}
