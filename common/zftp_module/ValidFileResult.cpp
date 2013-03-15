#include "LiteLib.h"
#include "zftp_protocol.h"
#include "ValidFileResult.h"
#include "ZCachedFile.h"
#include "zlc_util.h"
#include "ValidatedZCB.h"

ValidFileResult::ValidFileResult(ZCachedFile *zcf)
	: InternalFileResult(zcf)
{
}

DataRange ValidFileResult::get_available_data_range(ZFileRequest *req)
{
	return req->get_data_range().intersect(get_file_data_range());
}

uint32_t ValidFileResult::get_reply_size(ZFileRequest *req)
{
	uint32_t reply_size =
		sizeof(ZFTPDataPacket)
		+ sizeof(ZFTPMerkleRecord) * req->get_num_tree_locations()
		+ req->get_padding_request()
		+ get_available_data_range(req).size();
	return reply_size;
}

void ValidFileResult::fill_reply(Buf* vbuf, ZFileRequest *req)
{
	ZFTPDataPacket *zdp = (ZFTPDataPacket *) vbuf->data();
	zdp->header.code = z_htong(zftp_reply_data, sizeof(zdp->header.code));
	zdp->hash_len = z_htong(sizeof(hash_t), sizeof(zdp->hash_len));
	zdp->zftp_lg_block_size = z_htong(
		ZFTP_LG_BLOCK_SIZE,
		sizeof(zdp->zftp_lg_block_size));
	zdp->zftp_tree_degree = z_htong(
		ZFTP_TREE_DEGREE,
		sizeof(zdp->zftp_tree_degree));

	zdp->file_hash = zcf->file_hash;
	zdp->metadata = zcf->metadata;

	zdp->padding_size = z_htong(
		req->get_padding_request(), sizeof(zdp->padding_size));

	DataRange range = get_available_data_range(req);
	zdp->data_start = z_htong(range.start(), sizeof(zdp->data_start));
	zdp->data_end = z_htong(range.end(), sizeof(zdp->data_end));

	zdp->num_merkle_records = z_htong(
		req->get_num_tree_locations(),
		sizeof(zdp->num_merkle_records));

	ZFTPMerkleRecord *zmr = (ZFTPMerkleRecord *) (&zdp[1]);
	for (uint32_t idx=0; idx<req->get_num_tree_locations(); idx++)
	{
		uint32_t location = req->get_tree_location(idx);
		zmr[idx].tree_location =
			z_htong(location, sizeof(zmr[idx].tree_location));
		zmr[idx].hash = zcf->get_validated_merkle_record(location)->hash;
	}

	uint8_t *data = (uint8_t*) &zmr[req->get_num_tree_locations()];

	// insert requested padding
	memset(data, 0, req->get_padding_request());
	data += req->get_padding_request();

	zcf->read(vbuf, (data - vbuf->data()), range.start(), range.size());
}

bool ValidFileResult::is_error()
{
	return false;
}

uint32_t ValidFileResult::get_filelen()
{
	return zcf->get_filelen();
}

DataRange ValidFileResult::get_file_data_range()
{
	return zcf->get_file_data_range();
}

#if 0
void ValidFileResult::read_set(BlockSet *set, uint8_t *buf, uint32_t size)
{
	zcf->read_set(set, buf, size);
}
#endif

void ValidFileResult::read(Buf* vbuf, uint32_t offset, uint32_t size)
{
	zcf->read(vbuf, 0, offset, size);
}

bool ValidFileResult::is_dir()
{
	return zcf->is_dir();
}
