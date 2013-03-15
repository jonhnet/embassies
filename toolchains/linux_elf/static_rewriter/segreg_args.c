#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "segreg_args.h"
#include "args_lib.h"

void args_init(Args *args, int argc, char **argv)
{
	args->input = NULL;
	args->display_selector_accesses = false;
	args->display_seg_derefs = false;

	argc-=1; argv+=1;

	while (argc>0)
	{
		CONSUME_STRING("--input", args->input);
		CONSUME_BOOL("--display-selector-accesses", args->display_selector_accesses);
		CONSUME_BOOL("--display-seg-derefs", args->display_seg_derefs);
		char buf[500]; snprintf(buf, sizeof(buf), "Unknown argument: %s", argv[0]);
		check(false, buf);
	}

	check(args->input!=NULL, "--input <filename> required");
	check(args->display_selector_accesses || args->display_seg_derefs,
		"no display option requested");
}
