#include <stdio.h>
#include <alloca.h>
#include <stdint.h>

#include "LiteLib.h"
#include "cheesymalloc.h"
#include "zutex_sync.h"
#include "pal_abi/pal_abi.h"
#include "xax_util.h"

#define VERBOSE_SELECT_DEBUG 0

#if VERBOSE_SELECT_DEBUG
#include "debug.h"
#endif // VERBOSE_SELECT_DEBUG

void zutex_simple_wait(ZoogDispatchTable_v1 *xdt, uint32_t *zutex, uint32_t val)
{
	ZutexWaitSpec spec;
	spec.zutex = zutex;
	spec.match_val = val;
	(xdt->zoog_zutex_wait)(&spec, 1);
}

void zutex_simple_wake(ZoogDispatchTable_v1 *xdt, uint32_t *zutex)
{
	(xdt->zoog_zutex_wake)(zutex, *zutex, 1, 0, NULL, 0);
}

//////////////////////////////////////////////////////////////////////////////

// some magic inspired by linux cmpxchg that apparently
// convinces compiler to pretend the pointer is to something too-big-to-copy
struct dummy_struct
{
	uint32_t lots[1024];
};

static inline uint32_t cmpxchg(volatile uint32_t *ptr, uint32_t old, uint32_t new)
{
	uint32_t prev;
	__asm__ __volatile__(
		"lock cmpxchgl %1,%2"
		: "=a"(prev)
		: "r"(new), "m"(*((struct dummy_struct *)(ptr))), "0"(old)
		: "memory");
	return prev;
}

static inline uint32_t atomic_dec(uint32_t *ptr)
{
	uint32_t old_val;
	uint32_t replaced_val;
	do {
		old_val = *ptr;
		replaced_val = cmpxchg(ptr, old_val, old_val-1);
	} while (old_val != replaced_val);
	return old_val;
}

//////////////////////////////////////////////////////////////////////////////

// ZMutex based on mutex2 from http://www.akkadia.org/drepper/fudex.pdf

void zmutex_init(ZMutex *zmutex)
{
	zmutex->val = 0;
}

void zmutex_free(ZMutex *zmutex)
{
}

void zmutex_lock(ZoogDispatchTable_v1 *xdt, ZMutex *zmutex)
{
	int c;
	if ((c = cmpxchg(&zmutex->val, 0, 1)) != 0)
	{
		do {
			if (c==2 || cmpxchg(&zmutex->val, 1, 2)!=0)
			{
				zutex_simple_wait(xdt, &zmutex->val, 2);
			}
		} while ((c = cmpxchg(&zmutex->val, 0, 2)) != 0);
	}
}

void zmutex_unlock(ZoogDispatchTable_v1 *xdt, ZMutex *zmutex)
{
	if (atomic_dec(&zmutex->val) != 1)
	{
		zmutex->val = 0;
		zutex_simple_wake(xdt, &zmutex->val);
	}
}

//////////////////////////////////////////////////////////////////////////////

int _zas_check_internal(ZoogDispatchTable_v1 *xdt, ZAS **zasses, int count, ZutexWaitSpec *specs)
{
	int i=0;
	for (i=0; i<count; i++)
	{
		ZAS *zas = zasses[i];
		bool rc;
		rc = (zas->method_table->check)(xdt, zas, &specs[i]);
		if (rc) { return i; }
	}
	return -1;
}

int zas_check(ZoogDispatchTable_v1 *xdt, ZAS **zasses, int count)
{
	ZutexWaitSpec *dummyspecs = alloca(sizeof(ZutexWaitSpec) * count);
	return _zas_check_internal(xdt, zasses, count, dummyspecs);
}

int zas_wait_any(ZoogDispatchTable_v1 *xdt, ZAS **zasses, int count)
{
	ZutexWaitSpec *specs = alloca(sizeof(ZutexWaitSpec) * count);
	int index;
	while (1)
	{
		index = _zas_check_internal(xdt, zasses, count, specs);
		if (index >= 0)
		{
			break;
		}

		// call any prewait functions
		for (index=0; index<count; index++)
		{
			zas_prewait_t prewait = zasses[index]->method_table->prewait;
			if (prewait!=NULL)
			{
				prewait(xdt, zasses[index]);
			}
		}

		(xdt->zoog_zutex_wait)(specs, count);

		// call any postwait functions
		for (index=0; index<count; index++)
		{
			zas_postwait_t postwait = zasses[index]->method_table->postwait;
			if (postwait!=NULL)
			{
				postwait(xdt, zasses[index]);
			}
		}
	}
	return index;
}

//////////////////////////////////////////////////////////////////////////////

static bool _zevent_check(ZoogDispatchTable_v1 *xdt, ZAS *zas, ZutexWaitSpec *spec);
static ZASMethods3 _zevent_methods = { NULL, _zevent_check, NULL, NULL };

void zevent_init(ZEvent *zevent, bool auto_reset)
{
	zmutex_init(&zevent->mutex);
	zevent->auto_reset = auto_reset;
	zevent->seq_zutex = 0;
	zevent->signal = 0;
	zevent->zas.method_table = &_zevent_methods;
}

void zevent_free(ZEvent *zevent)
{
	// TODO need to count number of waiters
	// and signal them to get them to go away
	// before destroying this object.
	zmutex_free(&zevent->mutex);
}

static bool _zevent_check(ZoogDispatchTable_v1 *xdt, ZAS *zas, ZutexWaitSpec *spec)
{
	ZEvent *zevent = (ZEvent *) zas;
	zmutex_lock(xdt, &zevent->mutex);
	if (zevent->signal)
	{
		if (zevent->auto_reset)
		{
			zevent->signal = false;
		}
		zmutex_unlock(xdt, &zevent->mutex);
		return true;
	}
	uint32_t seq_val = zevent->seq_zutex;
	zmutex_unlock(xdt, &zevent->mutex);

	spec->zutex = &zevent->seq_zutex;
	spec->match_val = seq_val;
	return false;
}

void zevent_reset(ZoogDispatchTable_v1 *xdt, ZEvent *zevent)
{
	zmutex_lock(xdt, &zevent->mutex);
	zevent->signal = false;
	zmutex_unlock(xdt, &zevent->mutex);
}

void zevent_signal(ZoogDispatchTable_v1 *xdt, ZEvent *zevent, bool *dbg_oldstate)
{
	zmutex_lock(xdt, &zevent->mutex);
	if (dbg_oldstate)
	{
		*dbg_oldstate = zevent->signal;
	}
	if (!zevent->signal)
	{
		zevent->signal = true;
		zevent->seq_zutex += 1;
		int n_wake = (zevent->auto_reset) ? 1 : ZUTEX_ALL;
		(xdt->zoog_zutex_wake)(&zevent->seq_zutex, zevent->seq_zutex, n_wake, 0, NULL, 0);
	}
	zmutex_unlock(xdt, &zevent->mutex);
}

bool zevent_is_set(ZoogDispatchTable_v1 *xdt, ZEvent *zevent)
{
	bool result;
	zmutex_lock(xdt, &zevent->mutex);
	result = zevent->signal;
	zmutex_unlock(xdt, &zevent->mutex);
	return result;
}

//////////////////////////////////////////////////////////////////////////////

static bool _zpc_check(ZoogDispatchTable_v1 *xdt, ZAS *zas, ZutexWaitSpec *spec);
static ZASMethods3 _zpc_methods = { NULL, _zpc_check, NULL, NULL };

void zpc_init(ZProducerConsumer *zpc, char *debug_string)
{
	zpc->consumer_ready_half.zas.method_table = &_zpc_methods;
	zpc->consumer_ready_half.partner =  &zpc->producer_idle_half;
	lite_strcpy(zpc->consumer_ready_half.dbg_desc, debug_string);
	lite_strcat(zpc->consumer_ready_half.dbg_desc, ":C");
	zevent_init(&zpc->consumer_ready_half.event, true);
	zpc->producer_idle_half.zas.method_table = &_zpc_methods;
	zpc->producer_idle_half.partner =  &zpc->consumer_ready_half;
	lite_strcpy(zpc->producer_idle_half.dbg_desc, debug_string);
	lite_strcat(zpc->producer_idle_half.dbg_desc, ":P");
	zevent_init(&zpc->producer_idle_half.event, true);
}

void zpc_free(ZProducerConsumer *zpc)
{
	zevent_free(&zpc->consumer_ready_half.event);
	zevent_free(&zpc->producer_idle_half.event);
}

void zpc_signal_half(ZoogDispatchTable_v1 *xdt, ZPCHalf *half)
{
#if VERBOSE_SELECT_DEBUG
	{
		char buf[200];
		strcpy(buf, "_zpc_check: signal ");
		strcat(buf, half->dbg_desc);
		strcat(buf, "\n");
		debug_write_sync(buf);
	}
#endif // VERBOSE_SELECT_DEBUG
	bool oldval;
	zevent_signal(xdt, &half->event, &oldval);
#if VERBOSE_SELECT_DEBUG
	{
		char buf[200];
		strcpy(buf, "_zpc_check: `---signal ");
		strcat(buf, half->dbg_desc);
		strcat(buf, " was ");
		strcat(buf, oldval ? "true" : "false");
		strcat(buf, "\n");
		debug_write_sync(buf);
	}
#endif // VERBOSE_SELECT_DEBUG
}

static bool _zpc_check(ZoogDispatchTable_v1 *xdt, ZAS *zas, ZutexWaitSpec *spec)
{
	ZPCHalf *half = (ZPCHalf *) zas;
	bool rc = _zevent_check(xdt, &half->event.zas, spec);
#if VERBOSE_SELECT_DEBUG
	if (rc)
	{
		char buf[200];
		strcpy(buf, "_zpc_check: woke on ");
		strcat(buf, half->dbg_desc);
		strcat(buf, "\n");
		debug_write_sync(buf);
	}
#endif // VERBOSE_SELECT_DEBUG
	return rc;
}

ZAS *zpc_get_consumer_zas(ZProducerConsumer *zpc)
{
	return &zpc->consumer_ready_half.zas;
}

ZAS *zpc_get_producer_zas(ZProducerConsumer *zpc)
{
	return &zpc->producer_idle_half.zas;
}

//////////////////////////////////////////////////////////////////////////////

static bool _zar_check(ZoogDispatchTable_v1 *xdt, ZAS *zas, ZutexWaitSpec *spec);
static ZASMethods3 _zar_method_table = { NULL, _zar_check, NULL, NULL };

void zar_init(ZAlwaysReady *zar)
{
	zar->zas.method_table = &_zar_method_table;
}

void zar_free(ZAlwaysReady *zar)
{
}

static bool _zar_check(ZoogDispatchTable_v1 *xdt, ZAS *zas, ZutexWaitSpec *spec)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////

// This implementation is super-low-performance: every signal is a broadcast.

void zcond_init(ZCond *zcond)
{
	zmutex_init(&zcond->mutex);
	zcond->seq_zutex = 0;
}

void zcond_free(ZCond *zcond)
{
	zmutex_free(&zcond->mutex);
}

void zcond_signal(ZoogDispatchTable_v1 *xdt, ZCond *zcond)
{
	zcond_broadcast(xdt, zcond);
}

void zcond_broadcast(ZoogDispatchTable_v1 *xdt, ZCond *zcond)
{
	zmutex_lock(xdt, &zcond->mutex);
	zcond->seq_zutex += 1;
	(xdt->zoog_zutex_wake)(&zcond->seq_zutex, zcond->seq_zutex, ZUTEX_ALL, 0, NULL, 0);
	zmutex_unlock(xdt, &zcond->mutex);
}

// / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / /

static void _zcwr_virtual_destructor_cheesy(ZoogDispatchTable_v1 *xdt, ZAS *zas);
static void _zcwr_virtual_destructor_auto(ZoogDispatchTable_v1 *xdt, ZAS *zas);
static bool _zcwr_check(ZoogDispatchTable_v1 *xdt, ZAS *zas, ZutexWaitSpec *spec);
static void _zcwr_prewait(ZoogDispatchTable_v1 *xdt, ZAS *zas);
static void _zcwr_postwait(ZoogDispatchTable_v1 *xdt, ZAS *zas);
static ZASMethods3 _zcwr_method_table_auto = {
	_zcwr_virtual_destructor_auto,
	_zcwr_check,
	_zcwr_prewait,
	_zcwr_postwait
	};
static ZASMethods3 _zcwr_method_table_cheesy = {
	_zcwr_virtual_destructor_cheesy,
	_zcwr_check,
	_zcwr_prewait,
	_zcwr_postwait
	};

void _zcondwaitrecord_init_internal(ZCondWaitRecord *zcwr, ZoogDispatchTable_v1 *xdt, ZCond *zcond, ZMutex *zmutex)
{
	zcwr->zcond = zcond;
	zcwr->zmutex = zmutex;
	zmutex_lock(xdt, &zcond->mutex);
	zcwr->seq_value = zcond->seq_zutex;
	zmutex_unlock(xdt, &zcond->mutex);
}

ZCondWaitRecord *zcondwaitrecord_init_cheesy(CheesyMallocArena *arena, ZoogDispatchTable_v1 *xdt, ZCond *zcond, ZMutex *zmutex)
{
	ZCondWaitRecord *zcwr = (ZCondWaitRecord *) cheesy_malloc(arena, sizeof(ZCondWaitRecord));
	_zcondwaitrecord_init_internal(zcwr, xdt, zcond, zmutex);
	zcwr->zas.method_table = &_zcwr_method_table_cheesy;
	return zcwr;
}

void zcondwaitrecord_init_auto(ZCondWaitRecord *zcwr, ZoogDispatchTable_v1 *xdt, ZCond *zcond, ZMutex *zmutex)
{
	_zcondwaitrecord_init_internal(zcwr, xdt, zcond, zmutex);
	zcwr->zas.method_table = &_zcwr_method_table_auto;
}

void zcondwaitrecord_free(ZoogDispatchTable_v1 *xdt, ZCondWaitRecord *zcwr)
{
	zcwr->zas.method_table->virtual_destructor(xdt, &zcwr->zas);
}

static void _zcwr_virtual_destructor_auto(ZoogDispatchTable_v1 *xdt, ZAS *zas)
{
}

static void _zcwr_virtual_destructor_cheesy(ZoogDispatchTable_v1 *xdt, ZAS *zas)
{
	ZCondWaitRecord *zcwr = (ZCondWaitRecord *) zas;
	cheesy_free(zcwr);
}

static bool _zcwr_check(ZoogDispatchTable_v1 *xdt, ZAS *zas, ZutexWaitSpec *spec)
{
	ZCondWaitRecord *zcwr = (ZCondWaitRecord *) zas;
	zmutex_lock(xdt, &zcwr->zcond->mutex);
	uint32_t cur_seq_val = zcwr->zcond->seq_zutex;
	zmutex_unlock(xdt, &zcwr->zcond->mutex);
	if (cur_seq_val != zcwr->seq_value)
	{
		return true;
	}
	spec->zutex = &zcwr->zcond->seq_zutex;
	spec->match_val = zcwr->seq_value;
	return false;
}

static void _zcwr_prewait(ZoogDispatchTable_v1 *xdt, ZAS *zas)
{
	ZCondWaitRecord *zcwr = (ZCondWaitRecord *) zas;
	zmutex_unlock(xdt, zcwr->zmutex);
}

static void _zcwr_postwait(ZoogDispatchTable_v1 *xdt, ZAS *zas)
{
	ZCondWaitRecord *zcwr = (ZCondWaitRecord *) zas;
	zmutex_lock(xdt, zcwr->zmutex);
}

void zcond_wait(ZoogDispatchTable_v1 *xdt, ZCond *zcond, ZMutex *zmutex)
{
	ZCondWaitRecord zcwr;
	zcondwaitrecord_init_auto(&zcwr, xdt, zcond, zmutex);
	ZAS *zas = &zcwr.zas;
	zas_wait_any(xdt, &zas, 1);
	zcondwaitrecord_free(xdt, &zcwr);
}

