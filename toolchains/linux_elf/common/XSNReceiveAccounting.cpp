#include "XSNReceiveAccounting.h"
#include "LiteLib.h"

ZeroCopyBuf_XSNRA::ZeroCopyBuf_XSNRA(ZoogDispatchTable_v1 *xdt, ZoogNetBuffer *znb, XSNReceiveAccounting *xsnra)
	: ZeroCopyBuf_Xnb(xdt, znb, 0, znb->capacity),
	  xsnra(xsnra)
{
	xsnra->add(this);
}

ZeroCopyBuf_XSNRA::~ZeroCopyBuf_XSNRA()
{
	xsnra->remove(this);
	lite_assert(xnb!=NULL);	// be sure claim_xnb isn't screwing our counting.
}

uint32_t ZeroCopyBuf_XSNRA::hash(const void *v_a)
{
	return (uint32_t) v_a;
}

int ZeroCopyBuf_XSNRA::cmp(const void *v_a, const void *v_b)
{
	return ((uint32_t) v_a) - ((uint32_t) v_b);
}

//////////////////////////////////////////////////////////////////////////////

XSNReceiveAccounting::XSNReceiveAccounting(MallocFactory *mf, SyncFactory *sf)
{
	this->mutex = sf->new_mutex(false);
//	this->queue_depth = 0;
	hash_table_init(&ht, mf, ZeroCopyBuf_XSNRA::hash, ZeroCopyBuf_XSNRA::cmp);
}

void XSNReceiveAccounting::add(ZeroCopyBuf_XSNRA *buf)
{
	mutex->lock();
//	queue_depth += 1;
	hash_table_insert(&ht, buf);
	mutex->unlock();
}

void XSNReceiveAccounting::remove(ZeroCopyBuf_XSNRA *buf)
{
	mutex->lock();
//	lite_assert(queue_depth > 0);
//	queue_depth -= 1;
	hash_table_remove(&ht, buf);
	mutex->unlock();
}
