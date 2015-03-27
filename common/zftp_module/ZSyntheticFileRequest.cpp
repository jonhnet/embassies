#include "ZSyntheticFileRequest.h"
#include "xax_network_utils.h"

ZSyntheticFileRequest::ZSyntheticFileRequest(ZLCEmit *ze, MallocFactory *mf, hash_t *hash, SyncFactory *sf)
	: ZFileRequest(mf)
{
	_init(ze, hash, sf, 0, (uint32_t) -1);
}

ZSyntheticFileRequest::ZSyntheticFileRequest(ZLCEmit *ze, MallocFactory *mf, hash_t *hash, SyncFactory *sf, uint32_t data_start, uint32_t data_end)
	: ZFileRequest(mf)
{
	_init(ze, hash, sf, data_start, data_end);
}

void ZSyntheticFileRequest::_init(ZLCEmit *ze, hash_t *hash, SyncFactory *sf, uint32_t data_start, uint32_t data_end)
{
	event = sf->new_event(true);
	result = NULL;

	ZFTPRequestPacket _zrp;
	_zrp.hash_len = z_htong(sizeof(hash_t), sizeof(_zrp.hash_len));
	_zrp.url_hint_len = z_htong(0, sizeof(_zrp.url_hint_len));
	_zrp.file_hash = *hash;
	_zrp.num_tree_locations = z_htong(0, sizeof(_zrp.num_tree_locations));
	_zrp.padding_request = z_htong(0, sizeof(_zrp.padding_request));
	_zrp.data_start = z_htong(data_start, sizeof(_zrp.data_start));
	_zrp.data_end = z_htong(data_end, sizeof(_zrp.data_end));
	_zrp.compression_context.state_id = z_htong(ZFTP_NO_COMPRESSION, sizeof(_zrp.compression_context.state_id));
	_zrp.compression_context.seq_id = z_htong(0, sizeof(_zrp.compression_context.seq_id));
	load((uint8_t*) &_zrp, sizeof(_zrp));
	bool rc = parse(ze, true);
	lite_assert(rc);
}

ZSyntheticFileRequest::~ZSyntheticFileRequest()
{
	delete result;
	delete event;
}

void ZSyntheticFileRequest::deliver_result(InternalFileResult *result)
{
	this->result = result;
	lite_assert(this->result!=NULL);
	event->signal();
}

InternalFileResult *ZSyntheticFileRequest::wait_reply(int timeout_ms)
{
	if (result==NULL)
	{
		if (timeout_ms < 0)
		{
			event->wait();
		}
		else
		{
			event->wait(timeout_ms);
		}
	}
	return result;
}
