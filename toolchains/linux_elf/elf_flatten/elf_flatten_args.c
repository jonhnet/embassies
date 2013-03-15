#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "elf_flatten_args.h"
#include "args_lib.h"

void args_init(Args *args, int argc, char **argv)
{
	args->include_elf_header = false;
	args->insert_entry_jump = false;
	args->assert_rel_exists = false;
	args->allocate_new_stack_size = 0;
	args->input_image = NULL;
	args->insert_manifest = NULL;
	args->emit_c_dir = NULL;
	args->emit_c_symbol = NULL;
	args->emit_binary = NULL;

	argc-=1; argv+=1;

	while (argc>0)
	{
		CONSUME_BOOL("--include-elf-header", args->include_elf_header);
		CONSUME_BOOL("--insert-entry-jump", args->insert_entry_jump);
		CONSUME_HEXINT("--allocate-new-stack-size", args->allocate_new_stack_size);
		CONSUME_STRING("--input-image", args->input_image);
		CONSUME_STRING("--insert-manifest", args->insert_manifest);
		CONSUME_STRING("--emit-binary", args->emit_binary);
		CONSUME_STRING("--emit-c-dir", args->emit_c_dir);
		CONSUME_STRING("--emit-c-symbol", args->emit_c_symbol);
		char buf[500]; snprintf(buf, sizeof(buf), "Unknown argument: %s", argv[0]);
		check(false, buf);
	}
}
