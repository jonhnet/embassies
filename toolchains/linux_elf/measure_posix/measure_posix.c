#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <alloca.h>

#include "ElfArgs.h"
#include "ElfLoad.h"
#include "xe_mark_perf_point.h"

char loader[200<<10];

int main(int argc, char **argv)
{
	ElfLoaded el = load_elf("/lib/ld-linux.so.2");

	char arg_env_str[800];
	arg_env_str[0]='\0';
	int argi;
	for (argi=1; argi<argc; argi++)
	{
		char buf[80];
		snprintf(buf, sizeof(buf), "ARG %s\n", argv[argi]);
		strncat(arg_env_str, buf, sizeof(arg_env_str)-(strlen(arg_env_str)+1));
		fprintf(stderr, ":: %s", buf);
	}
	char display_env[800];
	snprintf(display_env, sizeof(display_env), "ENV DISPLAY=%s\n", getenv("DISPLAY"));
	strcat(arg_env_str, display_env);
	/*
	No, we don't want zoogy libwebkit, only zoogy midori; that has the
	posix tweaks in it.
	strcat(arg_env_str, "ENV LD_LIBRARY_PATH=" ZOOG_ROOT "/toolchains/linux_elf/lib_links");
	*/

	StartupContextFactory scf;

	uint32_t size_required = scf_count(&scf, arg_env_str, NULL);
	void *stack_setup = alloca(size_required);
	scf_fill(&scf, stack_setup);

	void *startFunc = (void*) (el.displace + el.ehdr->e_entry);

	posix_perf_point_setup();

	asm(
		"movl	%1,%%esp;"
		"movl	%0,%%eax;"
		"jmp	*%%eax;"
		:	/*output*/
		: "m"(startFunc), "m"(stack_setup)	/* input */
		: "%eax"	/* clobber */
	);
	return 0;
}
