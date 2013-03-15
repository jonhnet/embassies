#include <assert.h>

#include "zftp_protocol.h"
#include "ZFTPIndexConfig.h"
#include "ZDBDatum.h"

ZFTPIndexConfig::ZFTPIndexConfig()
{
	this->index_magic = 0x5844495a;	// 'ZIDX'
	this->version = 9;
	this->hash_len = sizeof(hash_t);
	this->zftp_lg_block_size = ZFTP_LG_BLOCK_SIZE;
	this->zftp_tree_degree = ZFTP_TREE_DEGREE;
}

ZDBDatum *ZFTPIndexConfig::get_config_key()
{
	static ZDBDatum *key = NULL;
	if (key==NULL)
	{
		key = new ZDBDatum("ZFTPIndexConfig");
	}
	return key;
}

ZDBDatum *ZFTPIndexConfig::get_config_value()
{
	assert(sizeof(*this)==5*sizeof(uint32_t));
	return new ZDBDatum((uint8_t*) this, sizeof(*this), false);
}
