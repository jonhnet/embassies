#include <assert.h>
#include <malloc.h>
#include <unistd.h>	// access()

#include "Monitor.h"
#include "Process.h"
#include "Transaction.h"
#include "standard_malloc_factory.h"

Process::Process(Monitor *m, uint32_t process_id, ZPubKey *pub_key)
	: m(m),
	  process_id(process_id),
	  xpbpool(new XPBPool(m->get_mf(), m->get_tunid())),
	  event_synchronizer(new EventSynchronizer()),
	  uieq(new UIEventQueue(m->get_mf(), m->get_sf(), event_synchronizer)),
	  pub_key(pub_key)
{
#if DEBUG_ZUTEX_OWNER
	dzt_init(&m->dzt);
#endif // DEBUG_ZUTEX_OWNER

	app_interface_init(&app_interface, m->get_netifc(), event_synchronizer);
	thread_table = new ThreadTable(m->get_mf());
	zqt = new ZQTable(m->get_mf());
//	blitter = new Blitter(event_synchronizer, xpbpool, m->get_xblit());

	m->get_blitter_manager()->register_palifc(this);

	pool_full_queue = new ZQ(m->get_mf(), -1);

	pthread_mutex_init(&pool_full_queue_mutex, NULL);

	labeled = false;
}

Process::Process(uint32_t process_id)
	: m(NULL),
	  process_id(process_id),
	  pub_key(NULL)
{
}

Process::~Process()
{
	if (m!=NULL)
	{
		m->get_blitter_manager()->unregister_palifc(this);
		int rc;
		delete pub_key;
		rc = pthread_mutex_destroy(&pool_full_queue_mutex);
		assert(rc==0);
		delete pool_full_queue;
		delete zqt;
		delete thread_table;
		app_interface_free(&app_interface, m->get_netifc());
	  	delete uieq;
		delete event_synchronizer;
		delete xpbpool;
		memset(this, 'Z', sizeof(*this));
	}
}

uint32_t Process::hash(const void *v_a)
{
	Process *p_a = (Process *) v_a;
	return p_a->process_id;
}

int Process::cmp(const void *a, const void *b)
{
	Process *p_a = (Process *) a;
	Process *p_b = (Process *) b;
	return p_a->process_id - p_b->process_id;
}

MThread *Process::lookup_thread(int thread_id)
{
	return thread_table->lookup(thread_id);
}

ZQ *Process::lookup_zq(uint32_t *zutex)
{
	return zqt->lookup(zutex);
}

bool Process::pid_is_live()
{
	char pid_path[1000];
	snprintf(pid_path, sizeof(pid_path), "/proc/%d/", process_id);
	int rc = access(pid_path, R_OK);
	return rc==0;
}

void Process::mark_labeled()
{	
	this->labeled = true;
}

const char *Process::get_label()
{
	if (!this->labeled)
	{
		return NULL;
	}
	return get_pub_key()->getOwner()->toString();
}
