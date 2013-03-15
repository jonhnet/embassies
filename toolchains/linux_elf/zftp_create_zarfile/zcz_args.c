#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "args_lib.h"
#include "zcz_args.h"

void list_append(LinkedList *ll, char *value)
{
	linked_list_insert_tail(ll, value);
}

void args_init(ZArgs *args, int argc, char **argv, MallocFactory *mf)
{
	args->log = NULL;
	args->zar = NULL;
	args->include_elf_syms = false;
	args->verbose = false;
	args->dep_file = NULL;
	linked_list_init(&args->overlay_list, mf);

	argc-=1; argv+=1;

	while (argc>0)
	{
		CONSUME_STRING("--log", args->log);
		CONSUME_STRING("--zar", args->zar);
		CONSUME_BOOL("--include-elf-syms", args->include_elf_syms);
		CONSUME_BOOL("--verbose", args->verbose);
		CONSUME_STRING("--dep-file", args->dep_file);
		CONSUME_BYFUNC("--overlay", list_append, &args->overlay_list);
		UNKNOWN_ARG();
	}
	check(args->log != NULL, "--log <input-log-filename> required");
	check(args->zar != NULL, "--zar <output-zar-filename> required");
}
