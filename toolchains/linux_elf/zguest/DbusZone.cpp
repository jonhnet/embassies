#include <malloc.h>
#include <unistd.h>
#include "DbusZone.h"
#include "LiteLib.h"
#include "RunConfig.h"

DbusZone::DbusZone()
	: Zone("dbus-daemon", 1, RunConfig::map_pie("dbus-daemon"))
{
	int rc;
	rc = pipe(pipes);
	lite_assert(rc==0);
	add_arg("--print-address");
	snprintf(argbuf, sizeof(argbuf), "%d", pipes[1]);
	add_arg(argbuf);
	add_arg("--session");
}

char *DbusZone::get_bus_address()
{
	int read_size = 256;
	char *connect_path = (char *)malloc(read_size);
	int rc;
	rc = read(pipes[0], connect_path, read_size);
	lite_assert(rc > 0 && rc<=read_size);
	lite_assert(connect_path[rc-1]=='\n');
	connect_path[rc-1] = '\0';
	return connect_path;
}

void DbusZone::polymorphic_break()
{
}

DbusClientZone::DbusClientZone(DbusZone *dbus, const char *label, int libc_brk_heap_mb, const char *app_path)
	: Zone(label, libc_brk_heap_mb, app_path),
	  dbus(dbus)
{
}

void DbusClientZone::synchronous_setup(LaunchState *ls)
{
	char envbuf[256];
	snprintf(envbuf, sizeof(envbuf), "DBUS_SESSION_BUS_ADDRESS=%s",
		dbus->get_bus_address());
	add_env(envbuf);
	Zone::synchronous_setup(ls);
}

