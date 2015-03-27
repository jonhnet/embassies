#include "ZCache.h"
#include "ZFileClient.h"
#include "ZFileServer.h"
#include "ZLookupServer.h"
#include "ZLookupClient.h"
#include "ZLCTestHarness.h"
#include "ZLCArgs.h"
#include "standard_malloc_factory.h"
#include "SyncFactory_Zutex.h"
#include "SocketFactory_Skinny.h"
#include "xax_skinny_network.h"
#include "zoog_malloc_factory.h"
#include "ZLCEmitXdt.h"
#include "ThreadFactory_Cheesy.h"
#include "format_ip.h"
#include "cheesy_snprintf.h"
#include "ZFetch.h"
#include "ZFastFetch.h"
#include "SendBufferFactory_Xnb.h"
#include "xax_util.h"
#include "ZCompressionStub.h"

#include "bulk_test.h"

class BulkTest {
public:
	void run(ZBCContext *context);
	void close();

private:
	ZLCArgs zlc_args;
	ZCache *zcache;

	ZBCContext *context;
	MallocFactory *mf;
};

void BulkTest::close()
{
	//ze->emit("BulkTest shutting down.\n");
	//delete zcache;
}

void BulkTest::run(ZBCContext *context)
{
	this->context = context;

	mf = context->mf;

	XIPifconfig ifconfig[2];
	int count = 2;
	(context->zdt->zoog_get_ifconfig)(ifconfig, &count);
	lite_assert(count==2);

	debug_logfile_append_f *g_debuglog = (debug_logfile_append_f *)
		(context->zdt->zoog_lookup_extension)("debug_logfile_append");
#define DISPLAY_UDPEP(aptr)	\
	{ char ipbuf[100], buf[200]; \
		format_ip(ipbuf, sizeof(ipbuf), &aptr->ipaddr); \
		cheesy_snprintf(buf, sizeof(buf), "%s: %s port %d\n", #aptr, ipbuf, Z_NTOHG(aptr->port)); \
		(g_debuglog)("stderr", buf); }

	zlc_args.origin_zftp =
		make_UDPEndpoint(mf, &ifconfig[1].local_interface, ZFTP_PORT);
	zlc_args.origin_lookup =
		make_UDPEndpoint(mf, &ifconfig[0].gateway, ZFTP_HASH_LOOKUP_PORT);

	DISPLAY_UDPEP(zlc_args.origin_zftp);
	DISPLAY_UDPEP(zlc_args.origin_lookup);

	SocketFactory *socket_factory = new SocketFactory_Skinny(context->xsn);
	SyncFactory *sf = new SyncFactory_Zutex(context->zdt);
	SendBufferFactory_Xnb *sbf = new SendBufferFactory_Xnb(context->zdt);
	zcache = new ZCache(&zlc_args, mf, sf, NULL, sbf, new ZCompressionStub());

	zcache->configure(NULL, NULL);

	ZLCEmit *ze = new ZLCEmitXdt(context->zdt, terse);
	ZLookupClient *zlookup_client;
	zlookup_client = new ZLookupClient(zlc_args.origin_lookup, socket_factory, sf, ze);
#if 0
	ZLCTestHarness *test_harness;
	ZLCEmit *ze = new ZLCEmitXdt(context->zdt);
	test_harness = new ZLCTestHarness(zcache, zlookup_client, 90, ze);
	delete test_harness;
#endif
#if 0
	ZFetch *zf = new ZFetch(zcache, zlookup_client);
	bool rc = zf->fetch("file:/home/sekiu/jonh/microsoft/xax/xax-svn/zoog/toolchains/linux_elf/lib_links/zoog.zarfile");
	lite_assert(rc);
	delete zf;
#endif
	ThreadFactory* tf = new ThreadFactory_Cheesy(context->zdt);

	PerfMeasure* perf = new PerfMeasure(context->zdt);

  uint8_t seed[RandomSupply::SEED_SIZE];
  context->zdt->zoog_get_random(sizeof(seed), seed);
  RandomSupply* random_supply = new RandomSupply(seed);

  uint8_t app_secret[SYM_KEY_BYTES];
  context->zdt->zoog_get_app_secret(sizeof(app_secret), app_secret);
  KeyDerivationKey* appKey = new KeyDerivationKey(app_secret, sizeof(app_secret));

	ZFastFetch *zff = new ZFastFetch(zcache->GetZLCEmit(), mf, tf, sf, zlookup_client, zlc_args.origin_zftp, socket_factory, perf, appKey, random_supply);
	ZeroCopyBuf *zcb = zff->fetch("test-vendor-name", "file:/home/sekiu/jonh/microsoft/xax/xax-svn/zoog/toolchains/linux_elf/lib_links/zoog.zarfile");
	lite_assert(zcb!=NULL);
	delete perf;
	delete zcb;
	delete zff;
	delete tf;

	close();
}

void bulk_test(ZBCContext *context)
{
	BulkTest *bulk_test = new BulkTest();

	// warm the big_cache
	for (int i=0; i<5; i++)
	{
		bulk_test->run(context);
	}

	for (int i=0; i<10; i++)
	{
		bulk_test->run(context);
	}

	block_forever(context->zdt);	// to capture profiler output
	(context->zdt->zoog_exit)();
}

uint32_t getCurrentTime() {
	lite_assert(false); // PAL currently doesn't know what time it is!
	return 0;
}

