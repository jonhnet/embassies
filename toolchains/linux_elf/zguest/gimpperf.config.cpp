#include <malloc.h>
#include <string.h>
#include "RunConfig.h"

void config_create(RunConfig *rc)
{
	rc->add_vnc();

// disabled so it won't wait for a click.
//	Zone *twm_zone = rc->add_app("twm");
//	(void) twm_zone;

	Zone *zone = rc->add_app("gimpperf");
	/*
	zone->add_arg("--no-data");
	zone->add_arg("--no-fonts");
	zone->add_arg("-i");
	*/

/*
	int zoog_root_env_len = strlen("ZOOG_ROOT=")+strlen(ZOOG_ROOT)+1;
	char* zoog_root_env = (char*) malloc(zoog_root_env_len);
	snprintf(zoog_root_env, zoog_root_env_len, "ZOOG_ROOT=%s", ZOOG_ROOT);
	zone->add_env(zoog_root_env);
*/
	(void) zone;
}

