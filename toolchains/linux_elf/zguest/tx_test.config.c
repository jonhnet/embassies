#include "zone_run.h"

void config_create(RunConfig *rc)
{
	run_config_add_zone(rc, "tx_test", 10, "/toolchains/linux_elf/fastpath_test/build/tx_test");
}

