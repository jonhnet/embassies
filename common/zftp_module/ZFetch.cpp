#include "ZFetch.h"
#include "ZSyntheticFileRequest.h"
#include "xax_network_utils.h"
#include "zftp_util.h"
#include "zlc_util.h"
#include "zftp_dir_format.h"
#include "MemBuf.h"

ZFetch::ZFetch(ZCache *zcache, ZLookupClient *zlookup_client)
{
	this->zcache = zcache;
	this->mf = zcache->mf;
	this->zlookup_client = zlookup_client;
	this->zreq = NULL;
	this->result = NULL;
}

ZFetch::~ZFetch()
{
	delete zreq;
}

bool ZFetch::fetch(const char *fetch_url)
{
	hash_t requested_hash = zlookup_client->lookup_url(fetch_url);
	hash_t zh = get_zero_hash();

	if (cmp_hash(&requested_hash, &zh)==0)
	{
		return false;
	}

	ZCachedFile *zcf = zcache->lookup_hash(&requested_hash);

	InternalFileResult *i_result = NULL;
	while (true)
	{
		ZLC_CHATTY(zcache->GetZLCEmit(), "ZFetch: requests %s.\n",, fetch_url);
		zreq = new ZSyntheticFileRequest(zcache->GetZLCEmit(), zcache->mf, &requested_hash, zcache->sf);
		zcf->consider_request(zreq);

		int timeout_ms = 1500;
		i_result = zreq->wait_reply(timeout_ms);
		if (i_result==NULL)
		{
			ZLC_TERSE(zcache->GetZLCEmit(), "ZFetch: timeout.\n");
			// darn, timeout.
			zcf->withdraw_request(zreq);
				// NB that may race with a result arriving, but that
				// just means we'll destruct the request without looking
				// at the result, which should be correct & nonleaky.
			delete zreq;
			continue;
		}
		break;
	}
	lite_assert(!i_result->is_error());
	result = (ValidFileResult *) i_result;
	payload_size = result->get_filelen();

	return true;
}

uint8_t *ZFetch::get_payload()
{
	uint8_t *payload = (uint8_t*) mf_malloc(mf, payload_size);
	MemBuf mb(payload, payload_size);
	result->read(&mb, 0, payload_size);
	return payload;
}

void ZFetch::free_payload(uint8_t *payload)
{
	mf_free(mf, payload);
}

uint8_t *ZFetch::_read(uint32_t *offset, uint32_t len)
{
	lite_assert(*offset + len <= get_payload_size());
	uint8_t *result = get_payload() + *offset;
	*offset += len;
	return result;
}

void ZFetch::dbg_display()
{
	uint8_t *payload = get_payload();
	if (result->is_dir())
	{
		uint32_t offset;
		for (offset=0; offset<get_payload_size(); )
		{
			// read the reclen field
			ZFTPDirectoryRecord *zdr = (ZFTPDirectoryRecord *) _read(&offset, sizeof(uint32_t));
			lite_assert(zdr->reclen < 1024);	// sanity-check length field
			_read(&offset, zdr->reclen-sizeof(uint32_t));
				// be sure the rest of the bytes are there; advance pointer
			ZLC_TERSE(zcache->GetZLCEmit(),
				"%s  %s\n",,
				zdr->type==ZT_DIR ? "dir " : (zdr->type==ZT_REG ? "file" : " ?? "),
				ZFTPDirectoryRecord_filename(zdr));
		}
	}
	else
	{
		if (get_payload_size()>0)
		{
#if ZLC_USE_PRINTF
			int rc = fwrite(payload, get_payload_size(), 1, stdout);
			lite_assert(rc==1);
#else
			ZLC_CHATTY(zcache->GetZLCEmit(),
				"Fetched %d-byte file\n",,
				get_payload_size());
#endif
		}
	}
	free_payload(payload);
}
