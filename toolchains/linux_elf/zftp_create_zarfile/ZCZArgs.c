#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "args_lib.h"
#include "ZCZArgs.h"
#include "zftp_protocol.h"

void list_append(LinkedList *ll, char *value)
{
	linked_list_insert_tail(ll, value);
}

void args_init(ZCZArgs *args, int argc, char **argv, MallocFactory *mf)
{
	args->trace = NULL;
	args->zar = NULL;
	args->include_elf_syms = false;
	args->verbose = false;
	args->hide_icons_dir = false;
	linked_list_init(&args->overlay_list, mf);
	args->placement_algorithm_enum = UNSET_PLACEMENT;
	args->zftp_lg_block_size = ZFTP_LG_BLOCK_SIZE;
	args->pack_small_files = true;

	argc-=1; argv+=1;

	while (argc>0)
	{
		CONSUME_STRING("--trace", args->trace);
		CONSUME_STRING("--zar", args->zar);
		CONSUME_BOOL("--include-elf-syms", args->include_elf_syms);
		CONSUME_BOOL("--verbose", args->verbose);
		CONSUME_BOOL("--hide-icons-dir", args->hide_icons_dir);
		CONSUME_BYFUNC("--overlay", list_append, &args->overlay_list);
		CONSUME_OPTION_ASSIGN("--greedy-placement",
			args->placement_algorithm_enum, UNSET_PLACEMENT, GREEDY_PLACEMENT);
		CONSUME_OPTION_ASSIGN("--stable-placement",
			args->placement_algorithm_enum, UNSET_PLACEMENT, STABLE_PLACEMENT);
		CONSUME_INT("--zftp-lg-block-size", args->zftp_lg_block_size);
		CONSUME_BOOL("--pack-small-files", args->pack_small_files);
			// NB you'd never really want to turn this off except for
			// experimental measurement.
		UNKNOWN_ARG();
	}
	check(args->trace != NULL, "--trace <input-trace-filename> required");
//	check(args->zar != NULL, "--zar <output-zar-filename> required");
	check(args->placement_algorithm_enum != UNSET_PLACEMENT, "placement algorithm required");
}
