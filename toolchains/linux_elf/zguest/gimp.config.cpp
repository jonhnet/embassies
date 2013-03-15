#include "RunConfig.h"

void config_create(RunConfig *rc)
{
	rc->add_vnc();

	Zone *twm_zone = rc->add_app("twm");
	(void) twm_zone;

	Zone *zone = rc->add_app("gimp");
	(void) zone;
}

