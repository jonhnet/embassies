#include <malloc.h>
#include <string.h>
#include "RunConfig.h"
#include "VncZone.h"

const char *app_repos_path = ZOOG_ROOT "/toolchains/linux_elf/lib_links/";

RunConfig::RunConfig(MallocFactory *mf, const char **incoming_envp)
{
	linked_list_init(&zones, mf);
	this->incoming_envp = incoming_envp;
}

Zone* RunConfig::add_vnc()
{
	VncZone *vncZone = new VncZone(&xvnc_start_detector);
	linked_list_insert_tail(&zones, vncZone);
	return vncZone;
}

char *RunConfig::map_pie(const char *appname)
{
	char *repos_path = (char*) malloc(
		strlen(app_repos_path) + strlen(appname)+strlen("-pie")+1);
	strcpy(repos_path, app_repos_path);
	strcat(repos_path, appname);
	strcat(repos_path, "-pie");
	return repos_path;
}

void RunConfig::add_zone(Zone *zone)
{
	linked_list_insert_tail(&zones, zone);
}

Zone *RunConfig::add_app(const char *appname)
{
	// An "app" is something that (a) lives in a standard place
	// in lib_links (so we can make it just from a makefile keyword), ...
	Zone *zone = new Zone(appname, 99, map_pie(appname));

	// ...and (b) needs to synchronize on vnc starting.
	zone->set_detector(&xvnc_start_detector);
	add_zone(zone);
	return zone;
}

#if 0
Zone *RunConfig::add_zone(const char *label, int libc_brk_heap_mb, const char *repos_path)
{
	char *path = (char*) malloc(200);	// TODO I <3 buffer overflows
	strcpy(path, ZOOG_ROOT);
	strcat(path, repos_path);

	Zone *zone = new Zone(label, libc_brk_heap_mb, path);
	add_zone(zone);
	return zone;
}
#endif
