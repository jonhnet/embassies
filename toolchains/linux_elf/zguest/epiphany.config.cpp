#include "RunConfig.h"
#include "DbusZone.h"

void config_create(RunConfig *rc)
{
	rc->add_vnc();

	DbusZone *dbus = new DbusZone();
	dbus->add_env("DBUS_VERBOSE=1");
	rc->add_zone(dbus);

	DbusClientZone *app = new DbusClientZone(
		dbus,
		"epiphany", 99, rc->map_pie("epiphany"));
//	app->add_env("DBUS_VERBOSE=1");
	app->set_detector(rc->get_xvnc_start_detector());
	rc->add_zone(app);
}
