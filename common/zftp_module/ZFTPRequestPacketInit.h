#pragma once
#include "zftp_protocol.h"

// A "constructor" for ZFTPRequestPacket, so that when we change the struct
// it's more obvious which call sites need to change.
void ZFTPRequestPacketInit(ZFTPRequestPacket* zrp,
	uint16_t url_hint_len,
	hash_t file_hash,
	uint32_t num_tree_locations,
	uint32_t padding_request,
	CompressionContext compression_context,
	uint32_t data_start,
	uint32_t data_end
	);
