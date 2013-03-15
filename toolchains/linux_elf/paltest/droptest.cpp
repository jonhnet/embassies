#include "droptest.h"
#include "simple_network.h"
#include "xax_network_utils.h"
#include "cheesy_snprintf.h"

#include "zemit.h"
#include "SendBufferFactory_Xnb.h"
#include "SocketFactory_Skinny.h"
#include "ThreadFactory_Cheesy.h"
#include "ZFileClient.h"
#include "ZLCTestHarness.h"

#include "droptest.h"

#define local_assert(x)	{ if (!(x)) { msg(#x); } lite_assert(x); }

enum { MAX_PAYLOAD=1500 };

DropTest::DropTest(
		ZoogDispatchTable_v1 *zdt,
		MallocFactory *mf,
		XaxSkinnyNetwork *xsn,
		XIPAddr *server)
	: zdt(zdt),
	  mf(mf),
	  xsn(xsn),
	  sf(xsn->get_timer_sf()),
	  ze(new ZLCEmitXdt(zdt, terse))
{
	zlcargs.origin_lookup =
		(UDPEndpoint*) mf_malloc(mf, sizeof(UDPEndpoint));
	zlcargs.origin_zftp =
		(UDPEndpoint*) mf_malloc(mf, sizeof(UDPEndpoint));
	ZLC_TERSE(ze, "ZLCVFS starts, calling evil\n");
	_setup_server_addrs(server, zlcargs.origin_lookup, zlcargs.origin_zftp);
	SendBufferFactory_Xnb *sbf = new SendBufferFactory_Xnb(zdt);
	zcache = new ZCache(&zlcargs, mf, sf, ze, sbf);

	socket_factory = new SocketFactory_Skinny(xsn);
	ThreadFactory *thread_factory = new ThreadFactory_Cheesy(zdt);

	zfile_client = new ZFileClient(zcache, zlcargs.origin_zftp, socket_factory, thread_factory, MAX_PAYLOAD);
	zcache->configure(NULL, zfile_client);

	zlookup_client = new ZLookupClient(zlcargs.origin_lookup, socket_factory, sf, ze);

	xsn->enable_random_drops(true);
}

DropTest::~DropTest()
{
	xsn->enable_random_drops(false);
}

void DropTest::run()
{
	ZLCTestHarness test_harness(zcache, zlookup_client, 100, ze);
}

void DropTest::_setup_server_addrs(
	XIPAddr *server,
	UDPEndpoint *out_lookup_ep,
	UDPEndpoint *out_zftp_ep)
{
	out_zftp_ep->ipaddr = *server;
	out_zftp_ep->port = z_htons(ZFTP_PORT);
	out_lookup_ep->ipaddr = *server;
	out_lookup_ep->port = z_htons(ZFTP_HASH_LOOKUP_PORT);
}
