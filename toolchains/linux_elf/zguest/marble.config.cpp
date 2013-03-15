#include "RunConfig.h"
#include "DbusZone.h"

void config_create(RunConfig *rc)
{
	rc->add_vnc();

	DbusZone *dbus = new DbusZone();
	//dbus->add_env("DBUS_VERBOSE=1");
	rc->add_zone(dbus);

	DbusClientZone *app = new DbusClientZone(
		dbus,
		"marble", 99, rc->map_pie("marble"));

#if 0
#define LIB_LINKS	ZOOG_ROOT "/toolchains/linux_elf/lib_links"
	const char *debug_libs = LIB_LINKS ":" LIB_LINKS "/debug_links";
	app->prepend_env(rc, "LD_LIBRARY_PATH", debug_libs);
#endif

	app->set_detector(rc->get_xvnc_start_detector());
	rc->add_zone(app);
}
