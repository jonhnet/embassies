#include "ZFTPRequestPacketInit.h"

void ZFTPRequestPacketInit(ZFTPRequestPacket* zrp,
	uint16_t url_hint_len,
	hash_t file_hash,
	uint32_t num_tree_locations,
	uint32_t padding_request,
	CompressionContext compression_context,
	uint32_t data_start,
	uint32_t data_end
	)
{
	zrp->hash_len = z_htong(sizeof(hash_t), sizeof(zrp->hash_len));
	zrp->url_hint_len = z_htong(url_hint_len, sizeof(zrp->url_hint_len));
	zrp->file_hash = file_hash;
	zrp->num_tree_locations = z_htong(num_tree_locations, sizeof(zrp->num_tree_locations));
	zrp->padding_request = z_htong(padding_request, sizeof(zrp->padding_request));
	zrp->compression_context.state_id = z_htong(compression_context.state_id, sizeof(zrp->compression_context.state_id));
	zrp->compression_context.seq_id = z_htong(compression_context.seq_id, sizeof(zrp->compression_context.seq_id));
	zrp->data_start = z_htong(data_start, sizeof(zrp->data_start));
	zrp->data_end = z_htong(data_end, sizeof(zrp->data_end));
}
