#include <string.h>
#include <stdlib.h>
#include "DTArgs.h"

DTArgs::DTArgs(int argc, char **argv)
{
	file[0] = NULL;
	file[1] = NULL;

	argc-=1; argv+=1;

	while (argc>0)
	{
		CONSUME_STRING("--file0", file[0]);
		CONSUME_STRING("--file1", file[1]);

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
