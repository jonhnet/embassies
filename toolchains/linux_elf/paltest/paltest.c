#include "paltest.h"
#include "segtest.h"
#include "VerifyLabelTest.h"
#include "uitest.h"
#include "nesttest.h"
#include "recvtest.h"
#include "threadtest.h"
#include "relocate_this.h"
#include "zoog_malloc_factory.h"

ZoogDispatchTable_v1 *g_zdt = 0;
debug_logfile_append_f *g_debuglog = 0;

extern void memtest();
extern void nettest();
extern void alarmtest();

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

	// NB if you use operator new (or maybe even before :v), you'll
	// be making zoog memory allocation calls. So, yeah, this kind
	// of un-units the unit tests. But it lets us use the crypto library,
	// which requires operator new.
	ZoogMallocFactory zmf;
	zoog_malloc_factory_init(&zmf, zdt);

	g_zdt = zdt;

	g_debuglog = (zdt->zoog_lookup_extension)("debug_logfile_append");
#define DO_TEST(which)	\
	(g_debuglog)("stderr", "Beginning " #which "tests.\n"); \
	which##test(zdt);

	DO_TEST(verifylabel)
	DO_TEST(nest)
	DO_TEST(ui)
	DO_TEST(thread)
	DO_TEST(seg)
	DO_TEST(alarm)
	DO_TEST(net)
	DO_TEST(mem)
	DO_TEST(recv)		// never terminates; not a unit test.

	(g_debuglog)("stderr", "All tests successfull.\n");
	(zdt->zoog_exit)();
}

void test_fail(void)
{
	(g_debuglog)("stderr", "A test failed.\n");
	(g_zdt->zoog_exit)();
}


