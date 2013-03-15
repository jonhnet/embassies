#include <string.h>
#include <stdlib.h>
#include "ZTArgs.h"

ZTArgs::ZTArgs(int argc, char **argv)
{
	zarfile = NULL;
	extract_path = NULL;
	dir_path = NULL;
	mode = unset;

	argc-=1; argv+=1;

	while (argc>0)
	{
		CONSUME_STRING("--zarfile", zarfile);
		CONSUME_ARG("--extract", extract_path, (check(mode==unset, "too many modes"), mode=extract, value));
		CONSUME_ARG("--dir", dir_path, (check(mode==unset, "too many modes"), mode=list_dir, value));
		CONSUME_OPTION_ASSIGN("--list-files", mode, unset, list_files);
		CONSUME_OPTION_ASSIGN("--list-chunks", mode, unset, list_chunks);

		char buf[500]; snprintf(buf, sizeof(buf), "Unknown argument: %s", argv[0]);
		check(false, buf);
	}

	check(zarfile, "must specify subject zarfile");
	check(mode != unset, "must specify a mode flag");
}
