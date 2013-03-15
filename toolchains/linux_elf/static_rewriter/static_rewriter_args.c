#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "static_rewriter_args.h"
#include "args_lib.h"

void args_init(Args *args, int argc, char **argv)
{
	args->input = NULL;
	args->output = NULL;
	args->dbg_symbols = false;
	args->dbg_conquer_mask = 0xffffffff;
	args->dbg_conquer_match = 0xffffffff;
	args->reloc_end_file = NULL;
	string_list_init(&args->ignore_funcs);
	args->keep_new_phdrs_vaddr_aligned = false;

	argc-=1; argv+=1;

	while (argc>0)
	{
		CONSUME_STRING("--input", args->input);
		CONSUME_STRING("--output", args->output);
		CONSUME_BOOL("--dbg-symbols", args->dbg_symbols);
		CONSUME_HEXINT("--dbg-conquer-mask", args->dbg_conquer_mask);
		CONSUME_HEXINT("--dbg-conquer-match", args->dbg_conquer_match);
		CONSUME_STRING("--reloc-end-file", args->reloc_end_file);
		CONSUME_BOOL("--keep_new_phdrs_vaddr_aligned", args->keep_new_phdrs_vaddr_aligned);
		{
			const char *argname = "--ignore-func";
			if (strcmp(argv[0], argname)==0)
			{
				char buf[500]; snprintf(buf, sizeof(buf), "%s missing arg", argname);
				check(argc>=2, buf);
				char *value = argv[1];
				string_list_add(&args->ignore_funcs, value);
				argc-=2; argv+=2; continue;
			}
		}

		UNKNOWN_ARG();
	}

	check(args->input!=NULL, "--input <filename> required");
	check(args->output!=NULL, "--output <filename> required");
}
