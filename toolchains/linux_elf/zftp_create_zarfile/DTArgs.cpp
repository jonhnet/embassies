#include <string.h>
#include <stdlib.h>
#include "DTArgs.h"
#include "zftp_protocol.h"

DTArgs::DTArgs(int argc, char **argv)
{
	print_hashes = false;
	position_dependent = false;
	file[0] = NULL;
	file[1] = NULL;
	list_blocks = false;
	blame_mismatches = false;

	argc-=1; argv+=1;

	while (argc>0)
	{
		CONSUME_STRING("--file0", file[0]);
		CONSUME_STRING("--file1", file[1]);
		CONSUME_OPTION("--print-hashes", print_hashes);
		CONSUME_OPTION("--position-dependent", position_dependent);
		CONSUME_OPTION("--list-blocks", list_blocks);
		CONSUME_OPTION("--blame-mismatches", blame_mismatches);

		char buf[500]; snprintf(buf, sizeof(buf), "Unknown argument: %s", argv[0]);
		check(false, buf);
	}

	for (int i=0; i<2; i++)
	{
		char buf[100];
		sprintf(buf, "--file%d not specified", i);
		check(file[i], buf);
	}
}
