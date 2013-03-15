#include "zone_run.h"

void config_create(RunConfig *rc)
{
	run_config_add_zone(rc, "proxytest", 10, "/toolchains/linux_elf/proxytest/build/proxytest");
}

