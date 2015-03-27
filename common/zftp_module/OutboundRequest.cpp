#include "xax_network_utils.h"
#include "OutboundRequest.h"
#include "ZFTPRequestPacketInit.h"

OutboundRequest::OutboundRequest(
	MallocFactory* mf,
	CompressionContext compression_context,
	const hash_t* file_hash,
	const char* url_hint,
	uint32_t num_tree_locations,
	uint32_t padding_request,
	DataRange data_range)
	: mf(mf)
{
	uint32_t url_hint_len = url_hint!=NULL ? lite_strlen(url_hint)+1 : 0;
	len = sizeof(ZFTPRequestPacket)
			+sizeof(uint32_t)*num_tree_locations
			+url_hint_len;
	zrp = (ZFTPRequestPacket *) mf_malloc(mf, len);
	ZFTPRequestPacketInit(zrp,
		url_hint_len,
		*file_hash,
		num_tree_locations,
		padding_request,
		compression_context,
		data_range.start(),
		data_range.end()
		);

	locations = (uint32_t*) &zrp[1];

	if (url_hint!=NULL)
	{
		char *url_hint_out = (char*) &(locations)[num_tree_locations];
		lite_memcpy(url_hint_out, url_hint, url_hint_len);
	}

	dbg_locations_remaining = num_tree_locations;
}

OutboundRequest::~OutboundRequest()
{
	mf_free(mf, zrp);
}

void OutboundRequest::set_location(uint32_t i, uint32_t location)
{
	locations[i] = z_htong(location, sizeof(locations[i]));
	dbg_locations_remaining -= 1;
}

ZFTPRequestPacket *OutboundRequest::get_zrp()
{
	lite_assert(dbg_locations_remaining == 0);
	return zrp;
}
