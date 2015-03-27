#include <assert.h>
#include <stdlib.h>
#ifdef _WIN32
#include <WinSock2.h>
#else
#include <sys/select.h>
#endif // _WIN32
#include "ZFTPApp.h"
#include "ZCache.h"
#include "ZFileClient.h"
#include "ZFileServer.h"
#include "ZLookupServer.h"
#include "ZLookupClient.h"
#include "ZLCTestHarness.h"
#include "ZLCArgs.h"
#include "ZFetch.h"
#include "standard_malloc_factory.h"
#include "SyncFactory.h"
#include "SocketFactory.h"
#include "ZLCEmitStdio.h"
#include "ThreadFactory.h"
#include "ZSyntheticFileRequest.h"
#include "SendBufferFactory_Memcpy.h"
#include "PerfMeasureIfc.h"
#include "ZCompressionImpl.h"

void ZFTPApp::close()
{
	if (zlc_args->print_net_stats)
	{
		fprintf(stdout, "Net receive %d bytes send %d bytes\n",
			socket_factory_counter->get_received_bytes(),
			socket_factory_counter->get_sent_bytes());
	}

	//fprintf(stderr, "App shutting down.\n");
	delete zcache;
	exit(0);
}

void ZFTPApp::fetch(const char* fetch_url)
{
	ZFetch zfetch(zcache, get_zlookup_client(), zlc_args->fetch_timeout_ms);
	bool rc = zfetch.fetch(fetch_url);
	if (!rc)
	{
		fprintf(stderr, "File not found\n");
	}
	else if (zlc_args->show_payload)
	{
		zfetch.dbg_display();
	}
}

ZLookupClient* ZFTPApp::get_zlookup_client()
{
	if (zlookup_client == NULL)
	{
		zlookup_client = new ZLookupClient(zlc_args->origin_lookup, socket_factory_counter, sf, ze);
	}
	return zlookup_client;
}

ZFTPApp::ZFTPApp(ZLCArgs *zlc_args,
	  MallocFactory *mf,
	  SocketFactory *underlying_socket_factory,
	  SyncFactory *sf,
	  ThreadFactory *tf,
	  SendBufferFactory *sbf)
	: zlc_args(zlc_args),
	  zlookup_client(NULL),
	  mf(mf),
	  sf(sf),
	  tf(tf),
	  zcompression(new ZCompressionImpl(mf, &zlc_args->compression_args)),
	  sbf(sbf)
{
	socket_factory_counter = new SocketFactory_Counter(underlying_socket_factory);
	ze = new ZLCEmitStdio(stderr, terse);
}

void ZFTPApp::run()
{
	zcache = new ZCache(zlc_args, mf, sf, ze, sbf, zcompression);

	ZFileClient *zclient = NULL;
	if (zlc_args->origin_zftp!=NULL)
	{
		zclient = new ZFileClient(zcache, zlc_args->origin_zftp, socket_factory_counter, tf, zlc_args->max_payload);
	}

	ZFileServer *zserver = NULL;
	if (zlc_args->listen_zftp!=NULL)
	{
		zserver = new ZFileServer(zcache, zlc_args->listen_zftp, socket_factory_counter, tf, new DummyPerfMeasure());
	}

	zcache->configure(zserver, zclient);

	ZLookupServer *zlookup_server = NULL;
	if (zlc_args->listen_lookup != NULL)
	{
		ZLCEmit *log_paths_emitter = NULL;
		if (zlc_args->log_paths!=NULL)
		{
			FILE *ofp;

#ifdef _WIN32
	fopen_s(&ofp, zlc_args->log_paths, "w");
#else 
	ofp = fopen(zlc_args->log_paths, "w");	
#endif

			
			log_paths_emitter = new ZLCEmitStdio(ofp, chatty);
		}
	
		zlookup_server = new ZLookupServer(zcache, zlc_args->listen_lookup, log_paths_emitter, socket_factory_counter, tf);
		(void) zlookup_server;
	}

	ZLCTestHarness *test_harness;
	if (zlc_args->run_test_harness > 0)
	{
		test_harness = new ZLCTestHarness(zcache, get_zlookup_client(), zlc_args->run_test_harness, ze);
		(void) test_harness;
		close();
	}

	if (zlc_args->dump_db)
	{
		zcache->debug_dump_db();
		close();
	}

	if (zlc_args->warm_url != NULL)
	{
		lite_assert(zlc_args->fetch_url != NULL);
		fetch(zlc_args->warm_url);
		socket_factory_counter->reset_stats();
		fetch(zlc_args->fetch_url);
		close();
	}

	if (zlc_args->fetch_url != NULL)
	{
		fetch(zlc_args->fetch_url);
		close();
	}

	if (zlc_args->probe)
	{
		hash_t zh = get_zero_hash();
		ZSyntheticFileRequest *zreq =
			new ZSyntheticFileRequest(ze, zcache->mf, &zh, sf);
		ZCachedFile *zcf = zcache->lookup_hash(zreq->get_hash());
		zcf->consider_request(zreq);
		delete zreq;
	}

	// we have no provision for graceful shutdown, so just block this
	// thread while the worker threads run.
	while (1)
	{
		select(0, NULL, NULL, NULL, NULL);
	}
}

