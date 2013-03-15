#include "packetfilter.h"
#include "zutex_sync.h"
#include "xax_util.h"
#include "xax_skinny_network.h"
#include "XStream.h"
#include "cheesy_snprintf.h"

// TODO packetfilter should be refactored to use zqueue.h instead
// of its own queue code. (zqueue.h IS this code, generalized.)

static bool _mq_check (ZoogDispatchTable_v1 *xdt, ZAS *zas, ZutexWaitSpec *spec);
static ZASMethods3 _mq_method_table = { NULL, _mq_check };

void mq_init(MessageQueue *mq, ZoogDispatchTable_v1 *xdt)
{
	mq->xdt = xdt;
	mq->zas.method_table = &_mq_method_table;
	zmutex_init(&mq->zmutex);
	mq->head.next = NULL;
	mq->head.zcb = NULL;
	mq->seq_zutex = 17;	// no special meaning. Zero's just so boring.
	mq->dbg_comment = 'i';
}

#define QUEUE_SANITY_CHECK 0
#if QUEUE_SANITY_CHECK
void queue_sanity_check(MessageQueue *mq)
{
	MQEntry *p0, *p1;
	int dizzy = 0;
	for (p0=&mq->head; p0!=NULL; p0=p0->next)
	{
		for (p1=p0->next; p1!=NULL; p1=p1->next)
		{
			xax_assert(p0!=p1);
			dizzy++;
			if (dizzy>100)
			{
				xax_assert(0);
			}
		}
	}
}
#else // QUEUE_SANITY_CHECK
static inline void queue_sanity_check(MessageQueue *mq) { }
#endif // QUEUE_SANITY_CHECK

void mq_append(MessageQueue *mq, MQEntry *msg)
{
	zmutex_lock(mq->xdt, &mq->zmutex);
	mq->dbg_comment = 'A';
	mq->seq_zutex += 1;
	if (mq->head.next==NULL)
	{
		// we were empty and (once we release the mutex) we won't be any longer.
		// signal one waiter, who will awaken, wait on the mutex,
		// and signal any further waiters if it leaves anything behind.
		zutex_simple_wake(mq->xdt, &mq->seq_zutex);
	}
	MQEntry *m;
	for (m=&mq->head; m->next!=NULL; m=m->next)
		{}	// TODO ridoculastic linear insert. Who WROTE this list, anyway?
	m->next = msg;
	xax_assert(msg->next==NULL);
	queue_sanity_check(mq);
	mq->dbg_comment = 'a';
	zmutex_unlock(mq->xdt, &mq->zmutex);
}

static bool _mq_check (ZoogDispatchTable_v1 *xdt, ZAS *zas, ZutexWaitSpec *spec)
{
	MessageQueue *mq = (MessageQueue *) zas;

	zmutex_lock(xdt, &mq->zmutex);
	mq->dbg_comment = 'C';
	bool result;
	if (mq->head.next != NULL)
	{
		result = true;
	}
	else
	{
		result = false;
		spec->zutex = &mq->seq_zutex;
		spec->match_val = mq->seq_zutex;
	}
	mq->dbg_comment = 'c';
	zmutex_unlock(xdt, &mq->zmutex);
	return result;
}

MQEntry *mq_pop(MessageQueue *mq, bool block)
{
	queue_sanity_check(mq);

	ZAS *zas = &mq->zas;
	MQEntry *msg = NULL;
	while (1)
	{
		int ready = zas_check(mq->xdt, &zas, 1);
		if (ready == 0)
		{
			zmutex_lock(mq->xdt, &mq->zmutex);
			mq->dbg_comment = 'P';
			if (mq->head.next == NULL)
			{
				// Huh, someone else grabbed it after my zas_check.
				// Go back around.
				mq->dbg_comment = '_';
				zmutex_unlock(mq->xdt, &mq->zmutex);
				continue;
			}

			// pop the message
			msg = mq->head.next;
			mq->head.next = msg->next;
			msg->next = NULL;

			if (mq->head.next != NULL)
			{
				// I just absorbed a wake, and took something,
				// and something else is left. Maybe someone else is waiting.
				// TODO excess use of zutex wakes; cleverer protocol could
				// stay out of the kernel (monitor) in the common case.

				zutex_simple_wake(mq->xdt, &mq->seq_zutex);
			}

			mq->dbg_comment = 'p';
			zmutex_unlock(mq->xdt, &mq->zmutex);
			break;
		}
		if (!block)
		{
			// nothing in the queue, but return without blocking
			break;
		}

		zas_wait_any(mq->xdt, &zas, 1);
	}
	queue_sanity_check(mq);
	return msg;
}

//////////////////////////////////////////////////////////////////////////////

PFHandler *pfhandler_init(XaxSkinnyNetworkPlacebo *xsn, uint8_t ipver, uint8_t ip_protocol, uint16_t udp_port)
{
	PFHandler *ph = (PFHandler*) mf_malloc((xsn->methods->get_malloc_factory)(xsn), sizeof(PFHandler));
	ph->registration.ipver = ipver;
	ph->registration.ip_protocol = ip_protocol;
	ph->registration.udp_port = udp_port;
	ph->xsn = xsn;
	mq_init(&ph->mq, (xsn->methods->get_xdt)(ph->xsn));
	return ph;
}

void pfhandler_free(PFHandler *ph)
{
	MallocFactory *mf = (ph->xsn->methods->get_malloc_factory)(ph->xsn);
	while (1)
	{
		MQEntry *msg = mq_pop(&ph->mq, false);
		if (msg==NULL)
		{
			break;
		}
		if (msg->zcb != NULL)
		{
			delete msg->zcb;
		}
		mf_free(mf, msg);
	}
	mf_free(mf, ph);
}

//////////////////////////////////////////////////////////////////////////////

static uint32_t _hash_registration(const void *a)
{
	PFRegistration *ra = (PFRegistration *) a;
	return ra->ipver << 24 | ra->ip_protocol << 16 | ra->udp_port;
}

static int _cmp_registration(const void *a, const void *b)
{
	// turns out _hash_registration loses no information.
	return _hash_registration(a) - _hash_registration(b);
}

void pftable_init(PFTable *pt, MallocFactory *mf)
{
	pt->mf = mf;
	hash_table_init(&pt->ht, mf, _hash_registration, _cmp_registration);
	cheesy_lock_init(&pt->pt_mutex);
}

void pftable_insert(PFTable *pt, PFHandler *ph)
{
	cheesy_lock_acquire(&pt->pt_mutex);
	hash_table_insert(&pt->ht, ph);
	cheesy_lock_release(&pt->pt_mutex);
}

void pftable_remove(PFTable *pt, PFHandler *ph)
{
	cheesy_lock_acquire(&pt->pt_mutex);
	hash_table_remove(&pt->ht, ph);
	cheesy_lock_release(&pt->pt_mutex);
}

PFHandler *pftable_lookup(PFTable *pt, PFRegistration *pr)
{
	PFHandler *ph;
	cheesy_lock_acquire(&pt->pt_mutex);
	ph = (PFHandler*) hash_table_lookup(&pt->ht, pr);
	cheesy_lock_release(&pt->pt_mutex);
	return ph;
}

static void _dump_one(void *u, void *a)
{
	XStream *xs = (XStream *) u;
	PFHandler *ph = (PFHandler *) a;
	char buf[100];
	cheesy_snprintf(buf, sizeof(buf), "hdlr{ipver=%d, ip_protocol=%d, udp_port %d}\n",
		ph->registration.ipver, ph->registration.ip_protocol, ph->registration.udp_port);
	xs->emit(xs, buf);
}

void pftable_dbg_dump(PFTable *pt, XStream *xs)
{
	hash_table_visit_every(&pt->ht, _dump_one, xs);
}
