#include "xax_util.h"

#include "ZQueue.h"

static bool _zq_check(ZoogDispatchTable_v1 *xdt, ZAS *zas, ZutexWaitSpec *spec);
static ZASMethods3 _zq_method_table = { NULL, _zq_check, NULL, NULL };

struct s_zqueue *zq_init(ZoogDispatchTable_v1 *xdt, MallocFactory *mf, consume_method_f *consume_method)
{
	ZQueue *zq = (ZQueue*) mf_malloc(mf, sizeof(ZQueue));
	zq->zas.method_table = &_zq_method_table;
	zq->seq_zutex = 9;	// zero is so ...derivative.
	zmutex_init(&zq->zmutex);
	zq->head.next = NULL;
	zq->tailp = &zq->head;
	zq->consume_method = consume_method;
	zq->xdt = xdt;
	zq->mf = mf;
	return zq;
}

void zq_free(ZQueue *zq)
{
	// TODO should check that no one is waiting on queue?
	MallocFactory *mf = zq->mf;
	ZQEntry *zqe;
	for (zqe=zq->head.next; zqe!=NULL; zqe=zqe->next)
	{
		xax_assert(0);	// TODO should deallocate user_storage
		mf_free(mf, zqe);
	}
	zmutex_free(&zq->zmutex);
	mf_free(mf, zq);
}

static bool _zq_check(ZoogDispatchTable_v1 *xdt, ZAS *zas, ZutexWaitSpec *spec)
{
	ZQueue *zq = (ZQueue *) zas;
	zmutex_lock(xdt, &zq->zmutex);
	bool result;
	if (zq->head.next != NULL)
	{
		result = true;
	}
	else
	{
		result = false;
		spec->zutex = &zq->seq_zutex;
		spec->match_val = zq->seq_zutex;
	}
	zmutex_unlock(xdt, &zq->zmutex);
	return result;
}

void zq_consume(ZQueue *zq, void *consume_request)
{
	MallocFactory *mf = zq->mf;
	ZAS *zas = &zq->zas;
	while (1)
	{
		int ready_idx = zas_check(zq->xdt, &zas, 1);
		if (ready_idx == 0)
		{
			zmutex_lock(zq->xdt, &zq->zmutex);
			if (zq->head.next==NULL)
			{
				// Someone grabbed it after my check. Loop around.
				zmutex_unlock(zq->xdt, &zq->zmutex);
				continue;
			}

			ZQEntry *zqe = zq->head.next;
			bool consumed = (zq->consume_method)(zqe->user_storage, consume_request);
			if (consumed)
			{
				zq->head.next = zqe->next;
				if (zq->head.next == NULL)
				{
					// we just consumed last message in queue,
					// which means tail needs to be reset.
					zq->tailp = &zq->head;
				}
				mf_free(mf, zqe);
			}

			if (zq->head.next!=NULL)
			{
				// there's still data to consume; maybe another waiter
				// would like it.
				zutex_simple_wake(zq->xdt, &zq->seq_zutex);
			}

			zmutex_unlock(zq->xdt, &zq->zmutex);
			break;
		}
		zas_wait_any(zq->xdt, &zas, 1);
	}
}

void zq_insert(ZQueue *zq, void *user_storage)
{
	ZQEntry *zqe = (ZQEntry *) mf_malloc(zq->mf, sizeof(ZQEntry));
	zqe->user_storage =user_storage;
	zqe->next = NULL;

	zmutex_lock(zq->xdt, &zq->zmutex);
	zq->seq_zutex += 1;
	if (zq->head.next==NULL)
	{
		zutex_simple_wake(zq->xdt, &zq->seq_zutex);
	}
	zq->tailp->next = zqe;
	zq->tailp = zqe;
	zmutex_unlock(zq->xdt, &zq->zmutex);
}

bool zq_is_empty(ZQueue *zq)
{
	return zq->head.next==NULL;
}

ZAS *zq_get_zas(ZQueue *zq)
{
	return &zq->zas;
}
