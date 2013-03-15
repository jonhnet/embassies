#include <string.h>
#include <stdio.h>
#include <elf.h>
#include <alloca.h>

#include "LiteLib.h"
#include "ElfLoad.h"
#include "pal_abi/pal_abi.h"
#include "ElfArgs.h"
#include "xax_auxv.h"

static void sc_new_counter(StartupContext *sc)
{
	sc->space = NULL;
	sc->capacity = 0;
	sc->ptr_len = 0;
	sc->argc = 0;
	sc->string_ptr = NULL;
	sc->string_len = 0;
	sc->envp = NULL;
}

static void sc_add(StartupContext *sc, size_t size, void *object)
{
	if (sc->space!=NULL)
	{
		//lite_assert(sc->space+sc->ptr_len+size <= sc->string_ptr);
		// can't do that assert meaningfully, since string_ptr moves.
		lite_memcpy(sc->space+sc->ptr_len, object, size);
	}
	sc->ptr_len+=size;
}

#define SC_ADD(sc, object)	sc_add(sc, sizeof(object), &(object))

static void add_auxv(StartupContext *sc, uint32_t type, uint32_t val)
{
	Elf32_auxv_t at;
	at.a_type = type;
	at.a_un.a_val = val;
	SC_ADD(sc, at);
}

typedef enum
{
	c_arg,
	c_env,
} CWhat;

typedef void (*ParseFunc)(void *this, CWhat what, const char *p, int len);

static void _parse_configuration(const char *configuration, ParseFunc func, void *arg)
{
	const char *p = configuration;
	while (1)
	{
		const char *n = lite_strchrnul(p, '\n');

		if (lite_memcmp(p, "ARG ", 4)==0)
		{
			func(arg, c_arg, p+4, n-(p+4));
		}
		else if (lite_memcmp(p, "ENV ", 4)==0)
		{
			func(arg, c_env, p+4, n-(p+4));
		}
		else if (p==n || *p=='\0' || *p=='#')
		{
			// comment / blank line
		}
		else
		{
			lite_assert(0);
		}

		if ((*n)=='\0')
		{
			break;
		}
		p = n+1;
	}
}

static void sc_copystring(StartupContext *sc, const char *p, int len)
{
	char *dest = sc->string_ptr;
	if (sc->space!=NULL)
	{
		lite_memcpy(sc->string_ptr, p, len);
		sc->string_ptr += len;
		sc->string_ptr[0] = '\0';
		sc->string_ptr += 1;
	}
	sc->string_len += (len+1);
	SC_ADD(sc, dest);
}

static void _populate_args(void *this, CWhat what, const char *p, int len)
{
	StartupContext *sc = (StartupContext *) this;
	if (what==c_arg)
	{
		sc->argc += 1;
		sc_copystring(sc, p, len);
	}
}

static void _populate_env(void *this, CWhat what, const char *p, int len)
{
	StartupContext *sc = (StartupContext *) this;
	if (sc->space!=NULL && sc->envp==NULL)
	{
		sc->envp = (const char**) (sc->space+sc->ptr_len);
	}
	if (what==c_env)
	{
		sc_copystring(sc, p, len);
	}
}

static void _fill_context(StartupContext *sc, const char *configuration, void *xax_dispatch_table)
{
	SC_ADD(sc, sc->argc);
	_populate_args((void *) sc, c_arg, "ld.so", 5);
	_parse_configuration(configuration, _populate_args, sc);
	char *nullp = NULL;
	SC_ADD(sc, nullp);
		// apparently callee expecting a NULL after args --
		// see dl-sysdep.c:72, where envp = argv + argc + 1 more.

	_parse_configuration(configuration, _populate_env, sc);
	SC_ADD(sc, nullp);

	add_auxv(sc, AT_PAGESZ, 0x00001000);
	add_auxv(sc, AT_CLKTCK, 0x00000064);
	add_auxv(sc, AT_FLAGS, 0x00000000);
	add_auxv(sc, AT_UID, 0x000003e8);
	add_auxv(sc, AT_EUID, 0x000003e8);
	add_auxv(sc, AT_GID, 0x000003e8);
	add_auxv(sc, AT_EGID, 0x000003e8);
	add_auxv(sc, AT_SECURE, 0x00000000);

	uint32_t xdt = (uint32_t) xax_dispatch_table;
	add_auxv(sc, AT_XAX_DISPATCH_TABLE, xdt);

/*
 * We'll modify our rtld to survive without sysinfo and hwcap
 * Well. That was easy. It seems to not have missed them.
	add_auxv(sc, AT_PLATFORM, 0xbfce880b);
	add_auxv(sc, AT_SYSINFO, 0xb7f59414);
	add_auxv(sc, AT_SYSINFO_EHDR, 0xb7f59000);
	add_auxv(sc, AT_HWCAP, 0xbfebfbff);
*/

	add_auxv(sc, AT_NULL, 0);
}

const char *sc_lookup_env(StartupContext *sc, const char *key)
{
	const char **p;
	for (p=sc->envp; *p!=NULL; p++)
	{
		if (lite_starts_with(key, *p))
		{
			char *equals = lite_index(*p, '=');
			if (equals==NULL)
			{
				lite_assert(false);	// env with no =
				return NULL;
			}
			return equals+1;
		}
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////

uint32_t scf_count(StartupContextFactory *scf, const char *configuration, void *xax_dispatch_table)
{
	scf->configuration = configuration;
	scf->xax_dispatch_table = xax_dispatch_table;
	sc_new_counter(&scf->count_context);
	_fill_context(&scf->count_context, scf->configuration, scf->xax_dispatch_table);
	return scf->count_context.ptr_len + scf->count_context.string_len;
}

void scf_fill(StartupContextFactory *scf, void *space)
{
	sc_new_counter(&scf->fill_context);

	scf->fill_context.space = space;
	scf->fill_context.capacity =
		scf->count_context.ptr_len + scf->count_context.string_len;	// caller must promise to have allocated enough space

	// put strings after the pointers
	scf->fill_context.string_ptr = ((char*) scf->fill_context.space)
		+ scf->count_context.ptr_len;

	// put pointers at the beginning, so they form the top of a stack
	// that's about to continue growing down.
	scf->fill_context.ptr_len = 0;

	// argc copy-in happens at beginning, so place it in from counted context
	scf->fill_context.argc = scf->count_context.argc;

	_fill_context(&scf->fill_context, scf->configuration, scf->xax_dispatch_table);
}

const char *scf_lookup_env(StartupContextFactory *scf, const char *key)
{
	return sc_lookup_env(&scf->fill_context, key);
}

bool scf_lookup_env_bool(StartupContextFactory *scf, const char *key)
{
	const char *val = scf_lookup_env(scf, key);
	if (val==NULL)
	{
		return false;
	}
	return (val[0]=='1');
}
