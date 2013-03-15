#include <stdio.h>
#include <assert.h>
#include "get_xax_dispatch_table.h"
#include "pal_abi/pal_abi.h"
#include "elf.h"
#include "xax_auxv.h"

int count_env_vector(const char **envp)
{
	void **envpc;
	int env_n = 0;
	for (envpc = (void**)envp; *envpc!=NULL; envpc++, env_n++)
	{
	}
	return env_n;
}

void *get_xax_dispatch_table(const char **incoming_envp)
{
	const char **ep = incoming_envp+count_env_vector(incoming_envp)+1;
	Elf32_auxv_t *auxv = (Elf32_auxv_t *) ep;

	int i;
	for (i=0; auxv[i].a_type!=AT_NULL; i++)
	{
		if (auxv[i].a_type == AT_XAX_DISPATCH_TABLE)
		{
			return (void*) auxv[i].a_un.a_val;
		}
	}
	assert(0);
	return (void*) -1;
}

