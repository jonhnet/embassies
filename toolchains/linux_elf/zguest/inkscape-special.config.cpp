#include "RunConfig.h"
void config_create(RunConfig *rc) {

	rc->add_vnc();

//	Zone *twm_zone = rc->add_app("twm");
//	(void) twm_zone;

	const char* appname = "inkscape";
	Zone *zone = new Zone(appname, 199, rc->map_pie(appname));
		// NB bigger brk. Stupid brk.
	zone->set_detector(rc->get_xvnc_start_detector());
	rc->add_zone(zone);
	zone->add_arg("/home/webguest/files/talk-slides.svg");
}
