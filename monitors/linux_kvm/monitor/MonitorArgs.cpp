#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "args_lib.h"
#include "MonitorArgs.h"

MonitorArgs::MonitorArgs(int argc, char **argv)
{
	delete_image_file = false;
	// image_file = ZOOG_ROOT "/toolchains/linux_elf/paltest/build/paltest.raw";
	image_file = ZOOG_ROOT "/toolchains/linux_elf/elf_loader/build/elf_loader.vendor_a.signed";
	wait_for_debugger = false;
	wait_for_core = true;

	argc-=1; argv+=1;

	while (argc>0)
	{
		CONSUME_BOOL("--delete-image-file", delete_image_file);
		CONSUME_STRING("--image-file", image_file);
		CONSUME_BOOL("--wait-for-debugger", wait_for_debugger);
		CONSUME_BOOL("--wait-for-core", wait_for_core);
		char buf[500]; snprintf(buf, sizeof(buf), "Unknown argument: %s", argv[0]);
		check(false, buf);
	}
}

