#include "RunConfig.h"

void config_create(RunConfig *rc)
{
	rc->add_vnc();

	Zone *zone = rc->add_app("xclock");
	zone->add_arg("-update");
	zone->add_arg("1");
}

