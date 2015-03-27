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

#include "zbig_main.h"

class App {
public:
	void run(ZBCContext *context);
	void close();

private:
	ZBCContext *context;
	ZLCArgs zlc_args;
	ZCache *zcache;

	MallocFactory *mf;
};

void App::close()
{
	//fprintf(stderr, "App shutting down.\n");
	delete zcache;
	(context->zdt->zoog_exit)();
}

void App::run(ZBCContext *context)
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
		make_UDPEndpoint(mf, &ifconfig[0].gateway, ZFTP_PORT);
	zlc_args.origin_lookup = 
		make_UDPEndpoint(mf, &ifconfig[0].gateway, ZFTP_HASH_LOOKUP_PORT);
	zlc_args.listen_lookup =
		make_UDPEndpoint(mf, &ifconfig[0].local_interface, ZFTP_HASH_LOOKUP_PORT);

	DISPLAY_UDPEP(zlc_args.origin_zftp);
	DISPLAY_UDPEP(zlc_args.origin_lookup);
//	DISPLAY_UDPEP(zlc_args.listen_zftp);
	DISPLAY_UDPEP(zlc_args.listen_lookup);

	SocketFactory *socket_factory = new SocketFactory_Skinny(context->xsn);
	SyncFactory *base_sf = new SyncFactory_Zutex(context->zdt);
	ZClock *zclock = new ZClock(context->zdt, base_sf);
	SyncFactory *sf = new SyncFactory_TimedZutex(context->zdt, zclock);
	SendBufferFactory_Xnb *sbf = new SendBufferFactory_Xnb(context->zdt);
	ZLCEmit *ze = new ZLCEmitXdt(context->zdt, terse);
	zcache = new ZCache(&zlc_args, mf, sf, ze, sbf, new ZCompressionStub());
	ThreadFactory *thread_factory = new ThreadFactory_Cheesy(context->zdt);
	PerfMeasureIfc* pmi = new PerfMeasure(context->zdt);
	zcache->configure_perf_measure_ifc(pmi);

	ZFileClient *zclient = NULL;
	if (zlc_args.origin_zftp!=NULL)
	{
		zclient = new ZFileClient(zcache, zlc_args.origin_zftp, socket_factory, thread_factory, zlc_args.max_payload);
	}

	ZFileServer *zserver4 = new ZFileServer(
		zcache,
		make_UDPEndpoint(mf, &ifconfig[0].local_interface, ZFTP_PORT),
		socket_factory,
		thread_factory,
		pmi);
	(void) zserver4;

	ZFileServer *zserver6 = new ZFileServer(
		zcache,
		make_UDPEndpoint(mf, &ifconfig[1].local_interface, ZFTP_PORT),
		socket_factory,
		thread_factory,
		pmi);
	(void) zserver6;

	// turns out zcache has no use for the ZServer.
	// Which is good, because I want to float two servers (ipv4, ipv6 sockets).
	// TODO remove it from state, to avoid confusion.
	zcache->configure(/*zserver*/ NULL, zclient);

	ZLookupServer *zlookup_server = NULL;
	if (zlc_args.listen_lookup != NULL)
	{
		zlookup_server = new ZLookupServer(zcache, zlc_args.listen_lookup, NULL, socket_factory, thread_factory);
		(void) zlookup_server;
	}

	if (zlc_args.dump_db)
	{
		zcache->debug_dump_db();
		close();
	}
}

void zbig_main(ZBCContext *context)
{
	App *app = new App();
	app->run(context);
}


