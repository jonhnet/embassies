#include "RunConfig.h"

void config_create(RunConfig *rc)
{
	rc->add_vnc();

#define LIB_LINKS	ZOOG_ROOT "/toolchains/linux_elf/lib_links"
	const char *gnucash_libs = LIB_LINKS "/usr_lib_gnucash" ":" LIB_LINKS "/usr_lib_gnucash/gnucash";

	Zone *twm_zone = rc->add_app("twm");
	(void) twm_zone;

	Zone *zone = rc->add_app("gnucash");
	zone->prepend_env(rc, "LD_LIBRARY_PATH", gnucash_libs);
	zone->prepend_env(rc, "GUILE_LOAD_PATH", LIB_LINKS "/usr_share_gnucash/guile-modules:" LIB_LINKS "/usr_share_gnucash/scm");
	zone->prepend_env(rc, "DYLD_LIBRARY_PATH", gnucash_libs);
	zone->prepend_env(rc, "GNC_MODULE_PATH", gnucash_libs);
}

