#include "xax_network_utils.h"
#include "zlc_util.h"
#include "ZFileRequest.h"

ZFileRequest::ZFileRequest(MallocFactory *mf)
{
	this->mf = mf;
}

ZFileRequest::ZFileRequest(
	MallocFactory *mf,
	uint8_t *bytes,
	uint32_t len)
{
	this->mf = mf;
	load(bytes, len);
}

void ZFileRequest::load(
	uint8_t *bytes,
	uint32_t len)
{
	this->len = len;
	this->bytes = (uint8_t*) mf_malloc(mf, len);
	lite_memcpy(this->bytes, bytes, len);
}

ZFileRequest::~ZFileRequest()
{
	mf_free(mf, bytes);
}

bool ZFileRequest::parse(ZLCEmit *ze, bool request_is_synthetic)
{
	this->request_is_synthetic = request_is_synthetic;

	uint16_t url_hint_len;

	ZLC_VERIFY(ze, len >= sizeof(ZFTPRequestPacket));
	zrp = (ZFTPRequestPacket *) bytes;
	ZLC_VERIFY(ze, Z_NTOHG(zrp->hash_len) == sizeof(hash_t));

	url_hint_len = Z_NTOHG(zrp->url_hint_len);
	num_tree_locations = Z_NTOHG(zrp->num_tree_locations);
	ZLC_VERIFY(ze,
		len >=
			sizeof(ZFTPRequestPacket)
			+ sizeof(uint32_t)*num_tree_locations
			+ url_hint_len);

	padding_request = Z_NTOHG(zrp->padding_request);

	data_range = DataRange(Z_NTOHG(zrp->data_start), Z_NTOHG(zrp->data_end));

	tree_locations = (uint32_t*) (&zrp[1]);

	url_hint = NULL;
	if (url_hint_len > 0)
	{
		url_hint = (char *) (&tree_locations[num_tree_locations]);
		ZLC_VERIFY(ze, url_hint[url_hint_len - 1] == '\0');
	}

	ZLC_CHATTY(ze, "Client requested %u bytes; locations:\n",,
		data_range.size());
	for (uint32_t i=0; i<num_tree_locations; i++)
	{
		ZLC_CHATTY(ze, "  location[%d] = %d\n",, i, get_tree_location(i));
	}

#if 0
// no longer server's business to prune requests; clients need to
// make sane requests.
	if (!request_is_synthetic && (block_count * ZFTP_BLOCK_SIZE > LENGTH_LIMIT))
	{
		uint32_t new_block_count = LENGTH_LIMIT/ZFTP_BLOCK_SIZE;
		ZLC_CHATTY(ze, "Client requested %u bytes; reducing request to %d\n",,
			block_count * ZFTP_BLOCK_SIZE,
			new_block_count * ZFTP_BLOCK_SIZE);
		block_count = new_block_count;
	}
#endif
	
	return true;

fail:
	return false;
}

uint32_t ZFileRequest::get_num_tree_locations()
{
	return num_tree_locations;
}

uint32_t ZFileRequest::get_tree_location(uint32_t idx)
{
	return Z_NTOHG(tree_locations[idx]);
}

bool ZFileRequest::is_semantically_equivalent(ZFileRequest *other)
{
	lite_assert(zrp->hash_len == other->zrp->hash_len);	// checked at ingestion
	if (cmp_hash(get_hash(), other->get_hash())!=0)
		{ return false; }
	if (get_num_tree_locations() != other->get_num_tree_locations())
		{ return false; }
	for (uint32_t i=0; i<get_num_tree_locations(); i++)
	{
		if (get_tree_location(i) != other->get_tree_location(i))
		{
			return false;
		}
	}
	if (get_padding_request() != other->get_padding_request())
		{ return false; }
	DataRange other_data_range = other->get_data_range();
	if (!get_data_range().equals(&other_data_range))
		{ return false; }
	return true;
}
