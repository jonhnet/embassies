#include "malloc_factory.h"
#include "ZFileReplyFromServer.h"
#include "zftp_protocol.h"
#include "zlc_util.h"

ZFileReplyFromServer::ZFileReplyFromServer(ZeroCopyBuf *zcb, ZLCEmit *ze, ZCompressionIfc* zcompression)
{
	this->_valid = false;
	this->zdp = NULL;
	this->zcb = zcb;
	this->_uncompressed_zcb = NULL;

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

	CompressionContext context;
	context.state_id = Z_NTOHG(zdp->compression_context.state_id);
	context.seq_id = Z_NTOHG(zdp->compression_context.seq_id);

	if (context.state_id != ZFTP_NO_COMPRESSION)
	{
		if (_payload_len==0)
		{
			// nothing to decompress; no stream there for zlib to read.
			// we'll leave _uncompressed_zcb==NULL, so get_payload()==_payload,
			// but that's okay, because get_payload_len()==0.
		}
		else
		{
			ZStreamIfc* zs =
				zcompression->get_stream(context, ZCompressionIfc::DECOMPRESS);
			// +1 because zlib is funny that way. :v(
			_uncompressed_zcb = zs->decompress(_payload, _payload_len, _data_range.size()+1);
			zs->advance_seq();
			if (_uncompressed_zcb->len() != _data_range.size())
			{
				ZLC_COMPLAIN(ze, "short decompression output\n");
				goto fail;
			}
		}
	}

	_valid = true;

fail:
	return;
}

ZFileReplyFromServer::~ZFileReplyFromServer()
{
	delete _uncompressed_zcb;
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
	if (_uncompressed_zcb!=NULL)
	{
		return _uncompressed_zcb->data();
	}
	else
	{
		return _payload;
	}
}

uint32_t ZFileReplyFromServer::get_payload_len()
{
	assert_valid();
	if (_uncompressed_zcb!=NULL)
	{
		return _uncompressed_zcb->len();
	}
	else
	{
		return _payload_len;
	}
}

