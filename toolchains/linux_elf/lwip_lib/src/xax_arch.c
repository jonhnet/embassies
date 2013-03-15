#include "lwip/sys.h"
#include "lwip/opt.h"
#include "lwip/stats.h"

#include "xax_arch.h"
#include "xaxif.h"

XaxMachinery g_xm;
	// TODO globals kick puppies, but I don't want to replumb all of lwip
	// to pass zdt around to all sem_new callsites just yet.

static u32_t
cond_wait(XaxMachinery *xm, ZCond *zcond, ZMutex *zmutex, u32_t timeout_ms, bool verbose);

struct sys_sem *
sys_sem_new_internal(XaxMachinery *xm, u8_t count)
{
	struct sys_sem *sem;


	sem = (struct sys_sem *) cheesy_malloc(&xm->arena, sizeof(struct sys_sem));
	if (sem != NULL) {
		sem->xm = xm;
		sem->c = count;
		zcond_init(&sem->zcond);
		zmutex_init(&sem->zmutex);
		sem->verbose = false;	// default no debug msgs
	}
	return sem;
}


void xax_arch_sem_configure(ZoogDispatchTable_v1 *zdt)
{
	g_xm.zdt = zdt;
	g_xm.zclock = xe_get_system_clock();
	concrete_mmap_xdt_init(&g_xm.cmx, zdt);
	cheesy_malloc_init_arena(&g_xm.arena, &g_xm.cmx.ami);
}

err_t
sys_sem_new(struct sys_sem **sem, u8_t count)
{
	SYS_STATS_INC_USED(sem);
	*sem = sys_sem_new_internal(&g_xm, count);
	if (*sem == NULL) {
		return ERR_MEM;
	}
	return ERR_OK;
}

static u32_t
cond_wait(XaxMachinery *xm, ZCond *zcond, ZMutex *zmutex, u32_t timeout_ms, bool verbose)
{
	int result;

	ZCondWaitRecord zcwr;
	zcondwaitrecord_init_auto(&zcwr, xm->zdt, zcond, zmutex);
	
	ZAS *zasses[2];
	zasses[0] = &zcwr.zas;
	int zascount = 1;
	LegacyZTimer *ztimer;
	bool using_ztimer = false;
	struct timespec start_time;

	if (timeout_ms > 0) {
		(xm->zclock->methods->read)(xm->zclock, &start_time);
		struct timespec timeout_offset;
		ms_to_timespec(&timeout_offset, timeout_ms);
		struct timespec timeout_absolute;
		timespec_add(&timeout_absolute, &start_time, &timeout_offset);

		ztimer = (xm->zclock->methods->new_timer)(xm->zclock, &timeout_absolute);
		zasses[1] = (xm->zclock->methods->get_zas)(ztimer);
		zascount += 1;
		using_ztimer = true;
	}

	if (verbose)
	{
		fprintf(stderr, "xax_arch cond_wait zcond %08x START\n", (uint32_t) zcond);
	}
	int idx = zas_wait_any(xm->zdt, zasses, zascount);
	if (verbose && idx==0)
	{
		fprintf(stderr, "xax_arch cond_wait zcond %08x SUCCEED\n", (uint32_t) zcond);
	}

	if (idx==1)
	{
		result = SYS_ARCH_TIMEOUT;
	}
	else if (timeout_ms > 0)
	{
		/* Calculate for how long we waited for the cond. */
		struct timespec end_time;
		(xm->zclock->methods->read)(xm->zclock, &end_time);
		struct timespec delay;
		timespec_subtract(&delay, &end_time, &start_time);
		result = timespec_to_ms(&delay);
	}
	else
	{
		result = SYS_ARCH_TIMEOUT;
	}

	if (using_ztimer)
	{
		(xm->zclock->methods->free_timer)(ztimer);
	}
	zcondwaitrecord_free(xm->zdt, &zcwr);

	return result;
}

u32_t
sys_arch_sem_wait(struct sys_sem **s, u32_t timeout_ms)
{
	u32_t time_needed = 0;
	struct sys_sem *sem;
	LWIP_ASSERT("invalid sem", (s != NULL) && (*s != NULL));
	sem = *s;

	zmutex_lock(sem->xm->zdt, &sem->zmutex);
	while (sem->c <= 0) {
		if (timeout_ms > 0) {
			time_needed = cond_wait(sem->xm, &sem->zcond, &sem->zmutex, timeout_ms, sem->verbose);

			if (time_needed == SYS_ARCH_TIMEOUT) {
				zmutex_unlock(sem->xm->zdt, &sem->zmutex);
				return SYS_ARCH_TIMEOUT;
			}
			/*			pthread_mutex_unlock(&(sem->mutex));
							return time_needed; */
		} else {
			cond_wait(sem->xm, &sem->zcond, &sem->zmutex, 0, sem->verbose);
		}
	}
	sem->c--;
	zmutex_unlock(sem->xm->zdt, &sem->zmutex);
	return time_needed;
}

void
sys_sem_signal(struct sys_sem **s)
{
	struct sys_sem *sem;
	LWIP_ASSERT("invalid sem", (s != NULL) && (*s != NULL));
	sem = *s;

	zmutex_lock(sem->xm->zdt, &sem->zmutex);
	if (sem->verbose)
	{
		fprintf(stderr, "xax_arch signal zcond %08x SIGNAL\n", (uint32_t) &sem->zcond);
	}
	sem->c++;

	if (sem->c > 1) {
		sem->c = 1;
	}

	zcond_broadcast(sem->xm->zdt, &sem->zcond);
	zmutex_unlock(sem->xm->zdt, &sem->zmutex);
}

void
sys_sem_free_internal(struct sys_sem *sem)
{
	zcond_free(&sem->zcond);
	zmutex_free(&sem->zmutex);
	cheesy_free(sem);
}

void
sys_sem_free(struct sys_sem **sem)
{
	if ((sem != NULL) && (*sem != SYS_SEM_NULL)) {
		SYS_STATS_DEC(sem.used);
		sys_sem_free_internal(*sem);
	}
}
