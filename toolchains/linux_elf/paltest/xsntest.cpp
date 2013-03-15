/*
 * These tests pull in the XaxSkinnyNetwork stack, which
 * (a) incorporates quite a lot more stuff than the other pal tests,
 * so we'd like to separate them out, and
 * (b) creates its own thread, and isn't currently tested to destruct
 * elegantly, so we want to be able to create one instance and use
 * it across multiple tests. Otherwise, we end up with heinousness
 * like the other thread calling free on an object allocated with an
 * arena that has gone out of scope(!).
 */
#include "xsntest.h"
#include "relocate_this.h"
#include "EventTimeoutTest.h"
#include "throughputtest.h"
#include "droptest.h"
#include "zoog_malloc_factory.h"

extern "C" {
void
elfmain(
	ZoogDispatchTable_v1 *zdt,
	uint32_t elfbase,
	uint32_t rel_offset,
	uint32_t rel_count);
}

void
elfmain(
	ZoogDispatchTable_v1 *zdt,
	uint32_t elfbase,
	uint32_t rel_offset,
	uint32_t rel_count)
{
	// First things first: fix up relocs so we can use globals & func ptrs
	// and pretty much everything else.
	relocate_this(elfbase, rel_offset, rel_count);

	ZoogMallocFactory zmf;
	zoog_malloc_factory_init(&zmf, zdt);
	XaxSkinnyNetwork xsn(zdt, zmf_get_mf(&zmf));

	XIPifconfig *ipv4_ifconfig = xsn.get_ifconfig(ipv4);
	XIPAddr hardcoded_server = ipv4_ifconfig->gateway;

	ZLCEmitXdt ze(zdt, terse);

	DropTest dt(zdt, zmf.mf, &xsn, &hardcoded_server);
	dt.run();

	EventTimeoutTest ett(&xsn, &ze);
	ett.run();

	ThroughputTest tt(zdt, &xsn, &hardcoded_server);
	tt.run();

	(zdt->zoog_exit)();
}
