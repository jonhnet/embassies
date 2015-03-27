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
	ZFTPDataPacket *zdp =
		(ZFTPDataPacket *) vbuf->write_header(sizeof(ZFTPDataPacket));

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

	CompressionContext context = vbuf->get_compression_context();
	zdp->compression_context.state_id = z_htong(context.state_id, sizeof(zdp->compression_context.state_id));
	zdp->compression_context.seq_id = z_htong(context.seq_id, sizeof(zdp->compression_context.seq_id));

	zdp->num_merkle_records = z_htong(
		req->get_num_tree_locations(),
		sizeof(zdp->num_merkle_records));

	ZFTPMerkleRecord *zmr = (ZFTPMerkleRecord *)
		(ZFTPMerkleRecord *) vbuf->write_header(sizeof(ZFTPMerkleRecord)*req->get_num_tree_locations());
	for (uint32_t idx=0; idx<req->get_num_tree_locations(); idx++)
	{
		uint32_t location = req->get_tree_location(idx);
		zmr[idx].tree_location =
			z_htong(location, sizeof(zmr[idx].tree_location));
		zmr[idx].hash = zcf->get_validated_merkle_record(location)->hash;
	}

	// insert requested padding
	uint8_t *padding =
		(uint8_t*) vbuf->write_header(req->get_padding_request());
	memset(padding, 0, req->get_padding_request());

#if DEBUG_COMPRESSION
	vbuf->debug_compression()->start_data(range.start(), range.size());
#endif // DEBUG_COMPRESSION
	zcf->write_blocks_to_buf(vbuf, 0, range.start(), range.size());
#if DEBUG_COMPRESSION
	vbuf->debug_compression()->end_data();
#endif // DEBUG_COMPRESSION

	// notice we go back and fill in the payload size once we know it
	// (after a possible compression on actual payload data)
	zdp->payload_size =
		z_htong(vbuf->get_payload_len(), sizeof(zdp->payload_size));
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

void ValidFileResult::write_payload_to_buf(Buf* vbuf, uint32_t data_offset, uint32_t size)
{
	zcf->write_blocks_to_buf(vbuf, 0, data_offset, size);
}

bool ValidFileResult::is_dir()
{
	return zcf->is_dir();
}
