#include "malloc_factory.h"
#include "ZFileReplyFromServer.h"
#include "zftp_protocol.h"
#include "zlc_util.h"

ZFileReplyFromServer::ZFileReplyFromServer(ZeroCopyBuf *zcb, ZLCEmit *ze)
{
	this->_valid = false;
	this->zdp = NULL;
	this->zcb = zcb;

	ZLC_VERIFY(ze, zcb->len() >= (int) sizeof(ZFTPReplyHeader));

	ZFTPReplyHeader *zrh;
	zrh = (ZFTPReplyHeader *) zcb->data();
	if (Z_NTOHG(zrh->code) != zftp_reply_data)
	{
		ZLC_COMPLAIN(ze, "reply error code %d\n",, Z_NTOHG(zrh->code));
		ZLC_VERIFY(ze, false);
	}
	
	ZLC_VERIFY(ze, zcb->len() >= (int) sizeof(ZFTPDataPacket));
	zdp = (ZFTPDataPacket *) zcb->data();

	num_merkle_records = Z_NTOHG(zdp->num_merkle_records);
	lite_assert(
		sizeof(ZFTPDataPacket)+sizeof(ZFTPMerkleRecord)*num_merkle_records
		<= zcb->len());
	raw_records = (ZFTPMerkleRecord *) (&zdp[1]);

	_payload = (uint8_t*) (&raw_records[num_merkle_records]);
	_payload += Z_NTOHG(zdp->padding_size);

	uint32_t prefix_bytes;
	prefix_bytes = (((uint8_t*)_payload) - ((uint8_t*) zdp));
	if (zcb->len() < prefix_bytes)
	{
		ZLC_COMPLAIN(ze, "short packet.\n");
		goto fail;
	}

	_payload_len = zcb->len() - prefix_bytes;

	_data_range = DataRange(Z_NTOHG(zdp->data_start), Z_NTOHG(zdp->data_end));

	_valid = true;

fail:
	return;
}

ZFileReplyFromServer::~ZFileReplyFromServer()
{
	delete zcb;
}

ZFTPFileMetadata *ZFileReplyFromServer::get_metadata()
{
	assert_valid();
	return &zdp->metadata;
}

hash_t *ZFileReplyFromServer::get_filehash()
{
	assert_valid();
	return &zdp->file_hash;
}

uint32_t ZFileReplyFromServer::get_merkle_tree_location(uint32_t i)
{
	return Z_NTOHG(get_merkle_record(i)->tree_location);
};

hash_t *ZFileReplyFromServer::get_merkle_hash(uint32_t i)
{
	assert_valid();
	lite_assert(i<get_num_merkle_records());
	return &get_merkle_record(i)->hash;
}

uint32_t ZFileReplyFromServer::get_num_merkle_records()
{
	assert_valid();
	return num_merkle_records;
}

ZFTPMerkleRecord *ZFileReplyFromServer::get_merkle_record(uint32_t i)
{
	assert_valid();
	lite_assert(i<num_merkle_records);
	return &raw_records[i];
}
uint8_t *ZFileReplyFromServer::get_payload()
{
	assert_valid();
	return _payload;
}

uint32_t ZFileReplyFromServer::get_payload_len()
{
	assert_valid();
	return _payload_len;
}

#if 0	// gonna have to change
uint8_t *ZFileReplyFromServer::access_block(uint32_t block_num, uint32_t *out_data_len)
{
	assert_valid();
	lite_assert(block_num >= get_first_block_num());
	uint32_t packet_block_idx = block_num - get_first_block_num();
	uint32_t data_len = ZFTP_BLOCK_SIZE;
	uint32_t payload_start = packet_block_idx*ZFTP_BLOCK_SIZE;
	uint32_t payload_end = (packet_block_idx+1)*ZFTP_BLOCK_SIZE;
	if (get_payload_len() < payload_start )
	{
		data_len = 0;
	}
	else if (get_payload_len() < payload_end)
	{
		data_len = get_payload_len() - payload_start;
	}
	*out_data_len = data_len;
	return _payload + payload_start;
}
#endif
