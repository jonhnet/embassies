#include "RunConfig.h"
#include "Zone.h"

void config_create(RunConfig *rc)
{
	Zone *zone = new Zone(
		"getaddr",	// just a label for debugging inside zguest
		10,			// MB of brk()-accessible address space.
		ZOOG_ROOT "/toolchains/linux_elf/getaddr_test/build/getaddr"
			// path to the -pie binary you want to run.
		);
	rc->add_zone(zone);
}


