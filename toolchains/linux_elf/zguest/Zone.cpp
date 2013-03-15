#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

#include "pal_abi/pal_abi.h"
#include "malloc_factory.h"

#include "launch_elf_interpreter.h"
#include "lwip_module.h"
#include "xax_extensions.h"
#include "invoke_debugger.h"
#include "standard_malloc_factory.h"
#include "LiteLib.h"

#include "Zone.h"
#include "RunConfig.h"

const char *ldlinux = ZOOG_ROOT "/toolchains/linux_elf/lib_links/ld-linux.so.2";

static uint32_t hash_env(const void *datum);
static int cmp_env(const void *a, const void *b);

Zone::Zone(const char *label, int libc_brk_heap_mb, const char *app_path)
{
	this->label = label;
	this->libc_brk_heap_mb = libc_brk_heap_mb;
	this->app_path = app_path;
	this->detector_ifc = NULL;
	this->break_before_launch = false;

	linked_list_init(&app_args, standard_malloc_factory_init());
	hash_table_init(&app_envs, standard_malloc_factory_init(),
		hash_env, cmp_env);
	
}

void Zone::synchronous_setup(LaunchState *ls)
{
	this->_ls = ls;

	snprintf(_ldso_brk_name, sizeof(_ldso_brk_name), "%s:ldso", label);
	xe_allow_new_brk(3<<12, _ldso_brk_name);

	snprintf(_libc_brk_name, sizeof(_libc_brk_name), "%s:libc", label);
	xe_allow_new_brk(libc_brk_heap_mb<<20, _libc_brk_name);
}

void zguest_break_point()
{
}

void Zone::set_break_before_launch(bool break_before_launch)
{
	this->break_before_launch = break_before_launch;
}

void Zone::run()
{
    fprintf(stderr, "run_app starting for zone \"%s\"\n", label);

	//INVOKE_DEBUGGER_RAW();

	if (detector_ifc != NULL)
	{
		detector_ifc->wait(_ls->xax_dispatch_table);
	}

	EnvBucket eb;

	eb_init(&eb);
	eb_add(&eb, eb_arg, app_path);

	LinkedListIterator lli;
	for (ll_start(&app_args, &lli);
		ll_has_more(&lli);
		ll_advance(&lli))
	{
		eb_add(&eb, eb_arg, (char*) ll_read(&lli));
	}

	char display_buf[60];
	sprintf(display_buf, "DISPLAY=:%d", VNC_PORT_NUMBER);
	_insert_default_env(display_buf);
	_insert_default_env(envp_find(_ls->incoming_envp, "HOME"));
	_insert_default_env(envp_find(_ls->incoming_envp, "PATH"));
	_insert_default_env(envp_find(_ls->incoming_envp, "LD_LIBRARY_PATH"));

// Yay! Our environment no longer requires a proxy.
//	_insert_default_env("http_proxy=http://172.31.40.21:80/");

	hash_table_visit_every(&app_envs, _install_env_in_eb, &eb);

	ElfLoaded elf_interp = load_elf(ldlinux);

	if (break_before_launch)
	{
		zguest_break_point();
	}
	polymorphic_break();
	launch_interpreter(
		&elf_interp,
		eb.string_space,
		_ls->xax_dispatch_table);
}

void Zone::_insert_default_env(const char *env)
{
	if (env==NULL)
	{
		return;
	}

	if (hash_table_lookup(&app_envs, (void*) env)!=NULL)
	{
		// let the app-defined value dominate this default.
		return;
	}
	// no app-supplied value; fill in the default.
	hash_table_insert(&app_envs, (void*) env);
}

void Zone::_install_env_in_eb(void *v_eb, void *v_datum)
{
	EnvBucket *eb = (EnvBucket *) v_eb;
	eb_add(eb, eb_env, (char*) v_datum);
}

void *Zone::_run_stub(void *v_this)
{
	((Zone*) v_this)->run();
	lite_assert(false);
	return NULL;	// doesn't.
}

void Zone::run_on_pthread()
{
	// use 128K stacks instead of default 8M.
	// This is only to make our vsize not look so embarassing,
	// since we have four threads that don't appear in the posix world.
	// This is an expedient for making non-distracting measurements;
	// in reality, we should assume that allocating address space we never
	// touch is free. But we don't have a nice way (with the present monitors)
	// to measure whether the space was ever actually touched,
	// so for now, we just try to guess a reasonable, small value.
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 128<<10);

	pthread_create(&thread, &attr, _run_stub, this);
	pthread_attr_destroy(&attr);
}

void Zone::add_arg(const char *arg)
{
	linked_list_insert_tail(&app_args, strdup(arg));
}

void Zone::add_env(const char *env)
{
	hash_table_insert(&app_envs, (void*) env);
}

void Zone::prepend_env(RunConfig *rc, const char *name, const char *val)
{
	const char *old_env = envp_find(rc->get_incoming_envp(), name);
	char *new_env = NULL;
	if (old_env==NULL)
	{
		new_env = (char*) malloc(strlen(name)+strlen(val)+3);
		sprintf(new_env, "%s=%s", name, val);
	}
	else
	{
		const char *old_val = index(old_env, '=');
		assert(old_val!=NULL);
		old_val += 1;
		int size = strlen(name)+1+strlen(val)+1+strlen(old_val)+1;
		new_env = (char*) malloc(size);
		sprintf(new_env, "%s=%s:%s", name, val, old_val);
	}
	add_env(new_env);
}

void Zone::set_detector(DetectorIfc *detector_ifc)
{
	lite_assert(this->detector_ifc==NULL);
	this->detector_ifc = detector_ifc;
}

int key_len(const void *vb)
{
	char *buf = (char*) vb;
	char *equals = lite_index(buf, '=');
	if (equals!=NULL)
	{
		return equals-buf;
	}
	else
	{
		return lite_strlen(buf);
	}
}

static uint32_t hash_env(const void *datum)
{
	return hash_buf(datum, key_len(datum));
}

static int cmp_env(const void *a, const void *b)
{
	return cmp_bufs(a, key_len(a), b, key_len(b));
}

void Zone::polymorphic_break()
{
}

ZoogDispatchTable_v1 *Zone::get_zdt()
{
	return _ls->xax_dispatch_table;
}
