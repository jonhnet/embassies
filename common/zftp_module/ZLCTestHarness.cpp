#include "ZCache.h"
#include "ZSyntheticFileRequest.h"
#include "reference_file.h"
#include "xax_network_utils.h"
#include "zftp_util.h"
#include "zlc_util.h"
#include "ZLCTestHarness.h"
#include "zftp_hash_lookup_protocol.h"
#include "ZLCArgs.h"
#include "ZFetch.h"

ZLCTestHarness::ZLCTestHarness(ZCache *zcache, ZLookupClient *zlookup_client, int max_selector, ZLCEmit *ze)
{
	this->zcache = zcache;
	this->zlookup_client = zlookup_client;
	this->max_selector = max_selector;
	this->ze = ze;

	_run_inner();
}

void ZLCTestHarness::_run_inner()
{
	int pattern_selector;
	for (pattern_selector=0; pattern_selector<max_selector; pattern_selector++)
	{
		for (int repeat=0; repeat<2; repeat++)
		{
			transfer_one(pattern_selector);
		}
	}
}

void ZLCTestHarness::transfer_one(int pattern_selector)
{
	ReferenceFile *rf = reference_file_init(zcache->mf, pattern_selector);
	lite_assert(pattern_selector==0 || rf->len > 0);
	ZLC_CHATTY(ze, "Transferring reference file #%d, len %d\n",,
		pattern_selector, rf->len);

	ZFetch zfetch(zcache, zlookup_client, ZFetch::DEFAULT_TIMEOUT);

	char url[100];
	cheesy_snprintf(url, sizeof(url), "%s%d", REFERENCE_FILE_SCHEME, rf->pattern_selector);
	zfetch.fetch(url);

	lite_assert(rf->len == zfetch.get_payload_size());
	uint8_t *payload = zfetch.get_payload();
	lite_assert(lite_memcmp(payload, rf->bytes, rf->len)==0);
	zfetch.free_payload(payload);

	ZLC_CHATTY(ze, "Pass #%d\n",, pattern_selector);
}
