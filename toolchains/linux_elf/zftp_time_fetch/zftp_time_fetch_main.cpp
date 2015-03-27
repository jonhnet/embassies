#include "ZCache.h"
#include "ZFileClient.h"
#include "ZFileServer.h"
#include "ZLookupServer.h"
#include "ZLookupClient.h"
#include "ZLCTestHarness.h"
#include "ZLCArgs.h"
#include "standard_malloc_factory.h"
#include "SyncFactory_Zutex.h"
#include "SyncFactory_TimedZutex.h"
#include "SocketFactory_Skinny.h"
#include "xax_skinny_network.h"
#include "zoog_malloc_factory.h"
#include "ZLCEmitXdt.h"
#include "ThreadFactory_Cheesy.h"
#include "format_ip.h"
#include "cheesy_snprintf.h"
#include "SendBufferFactory_Xnb.h"
#include "perf_measure.h"
#include "ZCompressionStub.h"
#include "ZFastFetch.h"
#include "ZLCEmit.h"
#include "FastFetchHelper.h"
#include "xax_util.h"
#include "cheesy_thread.h"
#include "zarfile.h"

#include "zftp_time_fetch_main.h"

class Measure
{
private:
	ZBCContext* context;
	PerfMeasure perf_measure;
	ZLCEmitXdt _ze, *ze;
	SyncFactory_Zutex _sf, *sync_factory;
	SocketFactory_Skinny socket_factory;
	ThreadFactory_Cheesy thread_factory;
	FastFetchHelper fast_fetch_helper;

	UDPEndpoint origin_lookup;
	UDPEndpoint origin_zftp;
	ZLookupClient* zlookup_client;
	UDPEndpoint fast_origin;
	RandomSupply* random_supply;
	KeyDerivationKey* appKey;
	uint32_t accum;

	void touch_all(void *addr, size_t len);

public:
	Measure(ZBCContext* context);
	void fetch_one();
	void root();
};

Measure::Measure(ZBCContext* context)
	: context(context),
	  perf_measure(context->zdt),
	  _ze(context->zdt, silent),
	  ze(&_ze),
	  _sf(context->zdt),
	  sync_factory(&_sf),
	  socket_factory(context->xsn),
	  thread_factory(context->zdt),
	  fast_fetch_helper(context->xsn, &socket_factory, ze),
	  accum(0)
{
	fast_fetch_helper._evil_get_server_addresses(&origin_lookup, &origin_zftp);

	zlookup_client = new ZLookupClient(
		&origin_lookup, &socket_factory, sync_factory, ze);

	bool rc = false;
	rc = fast_fetch_helper._find_fast_fetch_origin(&fast_origin);
	lite_assert(rc);

	fast_fetch_helper.init_randomness(context->zdt, &random_supply, &appKey);
}

void Measure::fetch_one()
{
	//perf_measure.mark_time("ish-start");
	ZFastFetch zff(
		ze,
		context->mf,
		&thread_factory,
		sync_factory,
		zlookup_client,
		&fast_origin,
		&socket_factory,
		&perf_measure,
		appKey,
		random_supply);
	const char* vendor_name = "zftp_time_fetch";
	const char* fetch_url = "file:" ZOOG_ROOT "/toolchains/linux_elf/zftp_time_fetch/test.zarfile";
	ZeroCopyBuf *zcb = zff.fetch(vendor_name, fetch_url);
	
	// NB len comes from reply packet header, not underlying ZNB
	touch_all(zcb->data(), zcb->len());

	if (zcb==NULL)
	{
		perf_measure.mark_time("ish-failed");
	}
	else
	{
		//perf_measure.mark_time("ish-done");
	}
}

void Measure::touch_all(void *addr, size_t len)
{
	uint32_t off = 0;
	for (off=0; off<len-sizeof(uint32_t); off+=4096)
	{
		uint32_t* ptr = (uint32_t*) (((uint8_t*)addr)+off);
		accum += ptr[0];
	}
}

void Measure::root()
{
	//fetch_one();	// collides with synchronous delay simulation in zftp_zoog
	perf_measure.mark_time("start");
	fetch_one();
	perf_measure.mark_time("done");
	(context->zdt->zoog_exit)();
}

void worker(void* v_context)
{
	ZBCContext* context = (ZBCContext*) v_context;
	Measure m(context);
	m.root();
}

void zftp_time_fetch_main(ZBCContext *context)
{
	// shift all work off main thread, to a cheesy_thread where
	// gs is valid. That's required for SyncFactory_TimedZutex.
	cheesy_thread_create(context->zdt, worker, context, 1<<16);
	block_forever(context->zdt);
}
