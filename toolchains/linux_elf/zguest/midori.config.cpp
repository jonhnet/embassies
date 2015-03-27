#include "RunConfig.h"
#include "DbusZone.h"
#include "AppIdentity.h"

void config_create(RunConfig *rc)
{
	Zone* vnczone = rc->add_vnc();
	vnczone->add_env("ZOOG_NO_TOPLEVEL=1");

	DbusZone *dbus = new DbusZone();
	//dbus->add_env("DBUS_VERBOSE=1");
	rc->add_zone(dbus);

	DbusClientZone *app = new DbusClientZone(
		dbus,
		"midori", 13, rc->map_pie("midori"));
	app->set_detector(rc->get_xvnc_start_detector());

	AppIdentity ai;

	char argbuf[200];
	snprintf(argbuf, sizeof(argbuf), 
		"file://" ZOOG_ROOT "/toolchains/linux_elf/apps/midori/%s.html",
		ai.get_identity_name());
	//app->add_arg("file://" ZOOG_ROOT "/toolchains/linux_elf/apps/midori/perf_test.html");
	app->add_arg(argbuf);
	rc->add_zone(app);
}
