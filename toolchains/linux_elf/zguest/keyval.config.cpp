#include "RunConfig.h"
#include "Zone.h"

void config_create(RunConfig *rc)
{
	Zone *zone = new Zone(
		"keyval",	// just a label for debugging inside zguest
		10,			// MB of brk()-accessible address space.
		ZOOG_ROOT "/toolchains/linux_elf/keyval_zoog/build/keyval-pie"
			// path to the -pie binary you want to run.
		);
	rc->add_zone(zone);
}


