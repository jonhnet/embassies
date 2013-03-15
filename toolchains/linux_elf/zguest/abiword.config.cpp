#include "RunConfig.h"

void config_create(RunConfig *rc)
{
	rc->add_vnc();

	Zone *zone = rc->add_app("abiword");
	zone->add_arg(ZOOG_ROOT "/toolchains/linux_elf/apps/abiword/atest.abw");
}

