#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include "pal_abi/pal_abi.h"
#include "malloc_factory.h"

#include "RunConfig.h"
#include "launch_elf_interpreter.h"
#include "lwip_module.h"
#include "xax_extensions.h"
#include "format_ip.h"
#include "zoog_malloc_factory.h"
#include "malloc_factory_operator_new.h"
#include "ambient_malloc.h"

int main(int argc, char **argv, const char **envp)
{
	LaunchState ls;
	ls.incoming_envp = envp;
#if 0
	ls.xax_dispatch_table =
		interpose_dispatch_table(get_xax_dispatch_table(envp));
#else
	ls.xax_dispatch_table =
		(ZoogDispatchTable_v1 *) get_xax_dispatch_table(envp);
#endif

	ZoogDispatchTable_v1 *zdt = (ZoogDispatchTable_v1*) ls.xax_dispatch_table;
	{
		XIPifconfig ifconfig;
		int count=1;
		(zdt->zoog_get_ifconfig)(&ifconfig, &count);
		lite_assert(count==1);
		char ipbuf[100];
		format_ip(ipbuf, sizeof(ipbuf), &ifconfig.local_interface);
		fprintf(stderr, "vncviewer %s:%d\n", ipbuf, 5900+VNC_PORT_NUMBER);
	}

	ZoogMallocFactory zmf;
	zoog_malloc_factory_init(&zmf, zdt);
	malloc_factory_operator_new_init(zmf_get_mf(&zmf));
	ambient_malloc_init(zmf_get_mf(&zmf));
	MallocFactory *mf = zmf.mf;

	RunConfig *rc = new RunConfig(mf, ls.incoming_envp);
	config_create(rc);
		// this is the call that gets 'overloaded' depending on which
		// .config.c we link this thing to.

	setup_lwip(ls.xax_dispatch_table, rc->get_xvnc_start_detector());

	LinkedListIterator lli;
	int zi;
	for (ll_start(rc->get_zones(), &lli), zi=0;
		ll_has_more(&lli);
		ll_advance(&lli), zi++)
	{
		Zone *zone = (Zone*) ll_read(&lli);
		zone->synchronous_setup(&ls);
		if (zi < rc->get_zones()->count-1)
		{
			zone->run_on_pthread();
			xe_wait_until_all_brks_claimed();
		}
		else
		{
			// Run "proper" app on main thread.
			// there once was some problem with apps
			// that were snooping in /proc and figuring out their thread ID and
			// getting angry. This is a quick workaround, but not terribly
			// sustainable; eventually we have to fake /proc anyway.
			// In fact, it may already have been properly fixed with improved
			// per-thread xprocfakery. But we'll keep the cargo worship
			// going for a bit here.
			zone->set_break_before_launch(true);
			zone->run();
		}
	}

	// Shouldn't actually return, but some #if'd-out test configurations did
	int i;
	for (i=0; i<60*60; i++)
	{
		sleep(1);
	}

	return -1;
}
