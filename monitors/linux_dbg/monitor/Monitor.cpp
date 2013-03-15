#include <fcntl.h>
#include <assert.h>
#include <malloc.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <fstream>
#include <sys/wait.h>

#include "Monitor.h"
#include "Transaction.h"
#include "standard_malloc_factory.h"
#include "ThreadFactory_Pthreads.h"
#include "Xblit.h"

using namespace std;

Monitor::Monitor()
	: service_thread_pool(this),
	  crypto(MonitorCrypto::WANT_MONITOR_KEY_PAIR)
{
	packet_account = NULL;

	mf = standard_malloc_factory_init();
	net_ifc_init_privileged(&netifc, this, &tunid);

	int rc;

	hash_table_init(&active_process_ht, mf, Process::hash, Process::cmp);
	hash_table_init(&idle_process_ht, mf, Process::hash, Process::cmp);
	hash_table_init(&stale_process_ht, mf, Process::hash, Process::cmp);

	dbg_allow_unlabeled_ui = getenv("DBG_ALLOW_UNLABELED_UI")!=0;

	// Bind xax_port_socket to listen to xaxified binary
	xp_addr.sun_family = PF_UNIX;
	sprintf(xp_addr.sun_path, "%s:%d", XAX_PORT_RENDEZVOUS, tunid.get_tunid());
	xp_addr.sun_path[sizeof(xp_addr.sun_path)-1] = '\0';

	rc = unlink(xp_addr.sun_path);
	if (!(rc==0 || errno==ENOENT))
	{
		fprintf(stderr, "Can't remove %s: %s.\n",
			xp_addr.sun_path, strerror(errno));
		assert(false);
	}

	xax_port_socket = socket(PF_UNIX, SOCK_DGRAM, 0);
	assert(xax_port_socket>=0);

	rc = fcntl(xax_port_socket, F_SETFD, FD_CLOEXEC);
	assert(rc==0);

	rc = bind(xax_port_socket, (struct sockaddr*) &xp_addr, sizeof(xp_addr));
	assert(rc==0);

	rc = chmod(xp_addr.sun_path, 0777);
	perror("chmod");
	assert(rc==0);

	pthread_t timer_thread;
	rc = pthread_create(&timer_thread, NULL, static_timer_loop, this);
	assert(rc==0);

	pthread_mutex_init(&mutex, NULL);

	MallocFactory *mf = standard_malloc_factory_init();
	ThreadFactory* tf = new ThreadFactory_Pthreads();
	Xblit *xblit = new Xblit(tf, get_sf());

	blitter_manager = new BlitterManager(xblit, mf, get_sf(), crypto.get_random_supply());

	service_thread_pool.grow();
}

void *Monitor::static_run(void *v_m)
{
	Monitor *m = (Monitor *) v_m;
	m->run();
	return NULL;
}

void Monitor::run()
{
	// blocks ready to service anyone -- want at least one of these
	// ready (or available promptly) at any given time.

	while (1)
	{
		Tx *tx = new Tx(this);
		service_thread_pool.departing();
		tx->handle();	// handle responsible to delete tx
		service_thread_pool.returning();

		bludgeon_zombies();
	}
}

void Monitor::bludgeon_zombies()
{
	int status;
	pid_t pid = waitpid(-1, &status, WNOHANG);
	if (pid > 0)
	{
		fprintf(stderr, "Child %d expires.\n", pid);
	}
}

void Monitor::start_new_thread()
{
	pthread_t *monitor_thread = (pthread_t*) malloc(sizeof(pthread_t));
	int rc;
	rc = pthread_create(monitor_thread, NULL, static_run, this);
	assert(rc==0);
}

// A clock algorithm pushes processes into the "stale" bucket;
// if they spend too long there, they get culled.
// TODO should probably ping stale processes by updating their
// HostAlarm or something, just to be sure they're not simply
// quiescent.

class DemoteSpec {
public:
	Monitor *m;
	HashTable *ht;
	const char *desc;
	DemoteSpec(Monitor *m, HashTable *ht, const char *desc)
		: m(m), ht(ht), desc(desc) {}

	static void _demote(void *user_obj, void *datum);
};

void DemoteSpec::_demote(void *user_obj, void *datum)
{
	DemoteSpec *ds = (DemoteSpec*) user_obj;
	Process *process = (Process *) datum;
	if (ds->desc!=NULL)
	{
		fprintf(stderr, "process %d moves to %s\n", process->dbg_get_process_id(), ds->desc);
	}
	if (ds->ht==NULL)
	{
		if (process->pid_is_live())
		{
			hash_table_insert(&ds->m->active_process_ht, process);
		}
		else
		{
			// we're inside a hash_table_remove_every, and a mutex,
			// so we don't want the mutex lock or the ht remove.
			ds->m->_disconnect_process_inner(process);
		}
	}
	else
	{
		hash_table_insert(ds->ht, process);
	}
}

void *Monitor::static_timer_loop(void *vm)
{
	Monitor *monitor = (Monitor *) vm;
	monitor->timer_loop();
	return NULL;
}

void Monitor::timer_loop()
{
	while (1)
	{
		pthread_mutex_lock(&mutex);

		{
			DemoteSpec ds(this, NULL, "free");
			hash_table_remove_every(&stale_process_ht, DemoteSpec::_demote, &ds);
		}
		{
			DemoteSpec ds(this, &stale_process_ht, "stale");
			hash_table_remove_every(&idle_process_ht, DemoteSpec::_demote, &ds);
		}
		{
			DemoteSpec ds(this, &idle_process_ht, NULL);
			hash_table_remove_every(&active_process_ht, DemoteSpec::_demote, &ds);
		}

		pthread_mutex_unlock(&mutex);

//		fprintf(stderr, "tick.\n");
		usleep(3000000);
	}
}


#if DEBUG_ZUTEX_OWNER
void dzt_init(DebugZutexThing *dzt)
{
	pthread_mutex_init(&dzt->mutex, NULL);
	dzt->ofp = fopen("dzt.log", "w");
}

void dzt_log(DebugZutexThing *dzt, XpHeader *xph, XaxFrame *xf, const char *msg)
{
	pthread_mutex_lock(&dzt->mutex);
	fprintf(dzt->ofp, "tid %8d op %08x : %s\n",
		xph->thread_id,
		xf->opcode,
		msg);
	fflush(dzt->ofp);
	pthread_mutex_unlock(&dzt->mutex);
}
#endif // DEBUG_ZUTEX_OWNER

Process *Monitor::connect_process(uint32_t process_id, ZPubKey *pub_key)
{
	Process *process;
	pthread_mutex_lock(&mutex);
	//_cull_stale_processes();

	process = new Process(this, process_id, pub_key);
	hash_table_insert(&active_process_ht, process);
//	exalted_process = process;
	pthread_mutex_unlock(&mutex);

	return process;
}

void Monitor::disconnect_process(Process *process)
{
	pthread_mutex_lock(&mutex);
	Process *p2 = _lookup_process_nolock(process);
		// NB that lookup step brings process up to the active table,
		// so we know the following delete will succeed.
	lite_assert(p2==process);
	hash_table_remove(&active_process_ht, process);
	_disconnect_process_inner(process);
	pthread_mutex_unlock(&mutex);
}

void Monitor::_disconnect_process_inner(Process *process)
{
	fprintf(stderr, "  disconnecting pid %d\n", process->get_id());
	delete process;
}

Process *Monitor::lookup_process(uint32_t process_id)
{
	Process *process = NULL;
	pthread_mutex_lock(&mutex);
	Process key(process_id);
	process = _lookup_process_nolock(&key);
	pthread_mutex_unlock(&mutex);
	return process;
}

Process *Monitor::_extract_from_demoted_table(HashTable *ht, Process *key)
{
	Process *process = (Process*) hash_table_lookup(ht, key);
	if (process!=NULL)
	{
		if (ht==&stale_process_ht)
		{
			fprintf(stderr, "Process %d returns to active\n", process->dbg_get_process_id());
		}
		hash_table_remove(ht, process);
		hash_table_insert(&active_process_ht, process);
	}
	return process;
}

Process *Monitor::_lookup_process_nolock(Process *key)
{
	Process *process = NULL;

	process = (Process*) hash_table_lookup(&active_process_ht, key);
	if (process==NULL)
	{
		process = _extract_from_demoted_table(&stale_process_ht, key);
		if (process==NULL)
		{
			process = _extract_from_demoted_table(&idle_process_ht, key);
		}
	}
	
	return process;
}

// TODO: Should probably lock this
void Monitor::get_random_bytes(uint8_t *out_bytes, uint32_t length) {
	crypto.get_random_supply()->get_random_bytes(out_bytes, length);
}

uint32_t getCurrentTime() {
  return (uint32_t) time (NULL);
}

