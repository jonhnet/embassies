#include "zone_run.h"

void config_create(RunConfig *rc)
{
    const char *ntpdate_args[2];
    ntpdate_args[0] = "127.0.0.1";
    ntpdate_args[1] = NULL;

	exit_detector_init(&rc->exit_detector);

    Zone *ntpdate_zone = run_config_add_zone(rc, "ntpdate", 10, "/toolchains/linux_elf/lib_links/ntpdate-pie");
	zone_add_arg(ntpdate_zone, "127.0.0.1");

	Zone *s3fs_zone = run_config_add_zone(rc, "s3fs", 10, "/toolchains/linux_elf/s3fs/build/s3fs");
	s3fs_zone->detector_ifc = &rc->exit_detector.detector_ifc;

	run_config_add_zone(rc, "writedoctest", 10, "/toolchains/linux_elf/writedoctest/build/writedoctest");
}

