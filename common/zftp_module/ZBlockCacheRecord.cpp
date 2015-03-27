#include <assert.h>
#include <string.h>		// memcpy, fast or lite-via-string_compat

#include "zftp_protocol.h"
#include "ZBlockCacheRecord.h"
#include "ZCache.h"
#include "math_util.h"

void ZBlockCacheRecord::_common_init(MallocFactory *mf, const hash_t *hash)
{
	this->mf = mf;
	this->data = (uint8_t *) mf_malloc(mf, ZFTP_BLOCK_SIZE);
	this->next_gap = 0;
	this->_valid = false;
	this->zcache = NULL;
	if (hash!=NULL)
	{
		this->hash = *hash;
	}
}

ZBlockCacheRecord::ZBlockCacheRecord(ZCache *zcache, const hash_t *expected_hash)
{
	_common_init(zcache->mf, expected_hash);
	this->zcache = zcache;
}

ZBlockCacheRecord::ZBlockCacheRecord(ZCache *zcache, uint8_t *in_data, uint32_t valid_data_len)
{
	_common_init(zcache->mf, NULL);
	this->zcache = zcache;
	_load_data_internal(in_data, valid_data_len, ZFTP_BLOCK_SIZE);
	zhash(this->data, ZFTP_BLOCK_SIZE, &hash);
	_valid = true;
}

#if ZLC_USE_PERSISTENT_STORAGE
ZBlockCacheRecord::ZBlockCacheRecord(MallocFactory *mf, const hash_t *hash, ZDBDatum *v)
{
	_common_init(mf, hash);
	v->read_raw(data, ZFTP_BLOCK_SIZE);
	this->_valid = true;
}
#endif // ZLC_USE_PERSISTENT_STORAGE

// hash_table key ctor. Not really a record, just a key.
ZBlockCacheRecord::ZBlockCacheRecord(const hash_t *hash)
{
	this->mf = NULL;
	this->data = NULL;
	this->hash = *hash;
}

ZBlockCacheRecord::~ZBlockCacheRecord()
{
	if (data!=NULL)
	{
		mf_free(mf, data);
	}
}

#if 0
void ZBlockCacheRecord::load_data(uint8_t *data, uint32_t valid_data_len)
{
	lite_assert(!_valid);	// should do this with naked records
	lite_assert(next_gap==0);	// should do this with naked records
	_load_data_internal(data, valid_data_len, 0, ZFTP_BLOCK_SIZE);
}
#endif

bool ZBlockCacheRecord::receive_data(
	uint32_t start,
	uint32_t end,
	ZFileReplyFromServer *reply,
	uint32_t offset)
{
	if (start < next_gap)
	{
		// just a duplicate packet.
		lite_assert(end <= next_gap);
		return false;
	}

	// can't yet handle data out-of-order.
	lite_assert(next_gap == start);

	uint32_t physical_data_available = min(reply->get_payload_len() - offset, end-start);
	_load_data_internal(reply->get_payload()+offset, physical_data_available, end-start);
	_validate();
	return true;
}

void ZBlockCacheRecord::complete_with_zeros()
{
	if (next_gap < ZFTP_BLOCK_SIZE)	// don't try to re-validate if we're already baked.
	{
		_load_data_internal(NULL, 0, ZFTP_BLOCK_SIZE - next_gap);
		_validate();
	}
}

void ZBlockCacheRecord::_load_data_internal(uint8_t *data_source, uint32_t physical_size, uint32_t logical_size)
{
	if (physical_size > 0)
	{
		lite_assert(next_gap + physical_size <= ZFTP_BLOCK_SIZE);
		lite_memcpy(this->data + next_gap, data_source, physical_size);
	}
	next_gap += physical_size;
	uint32_t zero_len = logical_size - physical_size;
	if (zero_len > 0)
	{
		lite_assert(next_gap + zero_len <= ZFTP_BLOCK_SIZE);
		lite_memset(this->data + next_gap, 0, zero_len);
	}
	next_gap += zero_len;
}

void ZBlockCacheRecord::get_first_gap(uint32_t *start, uint32_t *size)
{
	lite_assert(!_valid);
	*start = next_gap;
	*size = ZFTP_BLOCK_SIZE - next_gap;
}

void ZBlockCacheRecord::_validate()
{
	lite_assert(!_valid);
	if (next_gap < ZFTP_BLOCK_SIZE)
	{
		return;
	}
	lite_assert(next_gap == ZFTP_BLOCK_SIZE);
	hash_t actual_hash;
	zhash(this->data, ZFTP_BLOCK_SIZE, &actual_hash);

	lite_assert(cmp_hash(&actual_hash, &this->hash)==0);
	// Hash didn't match expectation! Need signaling path so higher
	// layers can recover from malicious data origin.

	_valid = true;
	zcache->store_block(this);
}

DataRange ZBlockCacheRecord::get_missing_range()
{
	if (_valid)
	{
		return DataRange();
	}
	else
	{
		return DataRange(next_gap, ZFTP_BLOCK_SIZE);
	}
}

void ZBlockCacheRecord::read(uint8_t *out_buf, uint32_t off, uint32_t len)
{
	lite_assert(_valid);
	lite_assert(off+len <= ZFTP_BLOCK_SIZE);
	memcpy(out_buf, this->data+off, len);
}

const uint8_t *ZBlockCacheRecord::peek_data()
{
	lite_assert(_valid);
	return data;
}

uint32_t ZBlockCacheRecord::__hash__(const void *a)
{
	const ZBlockCacheRecord *zbcr = (const ZBlockCacheRecord *) a;
	uint32_t *first_bytes = (uint32_t*) zbcr->hash.bytes;
	uint32_t hash_front = first_bytes[0];
	return hash_front;
}

int ZBlockCacheRecord::__cmp__(const void *a, const void *b)
{
	const ZBlockCacheRecord *zbcr_a = (const ZBlockCacheRecord *) a;
	const ZBlockCacheRecord *zbcr_b = (const ZBlockCacheRecord *) b;
	return cmp_hash(&zbcr_a->hash, &zbcr_b->hash);
}


