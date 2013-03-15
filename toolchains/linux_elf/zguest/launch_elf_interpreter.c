#include <stdio.h>
#include <elf.h>
#include <alloca.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "elf_aux_vect_syms.h"
#include "ElfArgs.h"
#include "launch_elf_interpreter.h"
#include "pal_abi/pal_abi.h"

#define USE_FILE_IO 1
#include "ElfLoad.h"


void launch_interpreter(ElfLoaded *elf_interp, const char *arg_env_str, void *xdt)
{
	StartupContextFactory scf;

	uint32_t size_required = scf_count(&scf, arg_env_str, xdt);
	void *stack_setup = alloca(size_required);
	scf_fill(&scf, stack_setup);

	void *startFunc = (void*) (elf_interp->displace + elf_interp->ehdr->e_entry);

	asm(
		"movl	%1,%%esp;"
		"movl	%0,%%eax;"
		"jmp	*%%eax;"
		:	/*output*/
		: "m"(startFunc), "m"(stack_setup)	/* input */
		: "%eax"	/* clobber */
	);
}


void eb_init(EnvBucket *eb)
{
	eb->next_space = eb->string_space;
}

void eb_add(EnvBucket *eb, EBType type, const char *assignment)
{
	if (assignment==NULL)
	{
		return;
	}
	int assignment_len = strlen(assignment);
	int required_len = 4 + assignment_len + 1;
	assert(required_len < eb->string_space+MAX_ENV_STR-eb->next_space - 1);

	const char *type_str;
	switch (type)
	{
		case eb_arg: type_str = "ARG "; break;
		case eb_env: type_str = "ENV "; break;
		default: assert(0);
	}

	strcpy(eb->next_space, type_str);
	strcat(eb->next_space, assignment);
	strcat(eb->next_space, "\n");
	eb->next_space += required_len;
}

const char *envp_find(const char **envp, const char *name)
{
	int nl = strlen(name);
	for (; *envp!=NULL; envp++)
	{
		if (strlen(*envp)>nl && memcmp(name, *envp, nl)==0 && (*envp)[nl]=='=')
		{
			return *envp;
		}
	}
	return NULL;
}

void envp_dump(char **envp)
{
	for (; *envp!=NULL; envp++)
	{
		printf("(%s)\n", *envp);
	}
	printf("(terminating NULL)\n");
	fflush(stdout);
}

void eb_dump(EnvBucket *eb)
{
	printf("%s", eb->string_space);
	fflush(stdout);
}

