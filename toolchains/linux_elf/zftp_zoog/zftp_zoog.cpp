#include "zftp_zoog.h"
#include "relocate_this.h"
#include "zbig_main.h"
#include "xax_util.h"
#include "bulk_test.h"

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

	ZBCContext context(zdt);

	zbig_main(&context);
//	bulk_test(&context);
	block_forever(context.zdt);
	(context.zdt->zoog_exit)();
}

