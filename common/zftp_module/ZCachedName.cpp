#include "LiteLib.h"
#include "ZCache.h"
#include "zlc_util.h"
#include "ZCachedName.h"
#include "ZFSOrigin.h"

ZCachedName::ZCachedName(ZCache *zcache, const char *url_hint, MallocFactory *mf)
{
	this->zcache = zcache;
	this->url_hint = mf_strdup(mf, url_hint);
	this->znr = NULL;
	this->mf = mf;
	this->status = zcs_ignorant;
	linked_list_init(&waiters, mf);

	mutex = zcache->sf->new_mutex(true);
}

ZCachedName::~ZCachedName()
{
	lite_assert(waiters.count==0);
	mf_free(this->mf, this->url_hint);
	delete znr;
	delete mutex;
}

hash_t *ZCachedName::known_validated_hash()
{
	hash_t *result;

	mutex->lock();

	_check_freshness();
	if (znr!=NULL)
	{
		result = znr->get_file_hash();
	}
	else
	{
		result = NULL;
	}

	mutex->unlock();
	return result;
}

void ZCachedName::consider_request(ZLookupRequest *req)
{
	mutex->lock();
	_consider_request_nolock(req);
	mutex->unlock();
}

void ZCachedName::_consider_request_nolock(ZLookupRequest *req)
{
	_check_freshness();

	if (znr == NULL)
	{
		zcache->probe_origins(url_hint);
	}

	if (znr == NULL)
	{
		ZLC_COMPLAIN(zcache->GetZLCEmit(), "no ZCachedName forwarding right now. Bit bucket.\n");
	}

	lite_assert(lite_strcmp(req->url_hint, this->url_hint)==0);
	if (znr==NULL)
	{
		ZLC_CHATTY(zcache->GetZLCEmit(), "ZCachedName::_consider queueing request\n");
		linked_list_insert_tail(&waiters, req);
	}
	else
	{
		req->deliver_reply(znr);
	}
}

void ZCachedName::_check_freshness()
{
	if (this->status == zcs_ignorant)
	{
		ZNameRecord *znr = NULL;
		znr = zcache->retrieve_name(url_hint);
		if (znr!=NULL)
		{
			_install(znr, zcs_clean);
		}
	}

	if (znr!=NULL && !znr->check_freshness(url_hint))
	{
		// this doesn't delete record from DB, but we're about
		// to re-probe origins below. Flaky? Yes. Works? Don't know
		// yet. Not sure what an elegant flowchart would look like.
		delete znr;
		znr = NULL;
	}
}

void ZCachedName::install_from_origin(ZNameRecord *znr)
{
	mutex->lock();
	_install(znr, zcs_dirty);
	mutex->unlock();
}

void ZCachedName::_install(ZNameRecord *new_znr, ZCachedStatus new_status)
{
	if (this->znr!=NULL)
	{
		delete this->znr;
	}
	this->znr = new ZNameRecord(new_znr);
	zcache->store_name(url_hint, znr);

	_update_status(new_status);
	
	_signal_waiters();
}

void ZCachedName::_signal_waiters()
{
	LinkedList private_waiters;
	lite_memcpy(&private_waiters, &this->waiters, sizeof(LinkedList));
	linked_list_init(&this->waiters, this->mf);

	while (private_waiters.count>0)
	{
		ZLookupRequest *request =
			(ZLookupRequest*) linked_list_remove_head(&private_waiters);
		_consider_request_nolock(request);
	}
}

uint32_t ZCachedName::__hash__(const void *a)
{
	// TODO use a cheaper hash, dude.
	ZCachedName *znr = (ZCachedName *) a;
	hash_t name_hash;
	zhash((uint8_t*) znr->url_hint, lite_strlen(znr->url_hint), &name_hash);
	// Boy, that struct-aliasing rule is a real hassle!
	uint32_t result;
	memcpy(&result, &name_hash, sizeof(result));
	return result;
}

int ZCachedName::__cmp__(const void *a, const void *b)
{
	ZCachedName *znr_a = (ZCachedName *) a;
	ZCachedName *znr_b = (ZCachedName *) b;
	return lite_strcmp(znr_a->url_hint, znr_b->url_hint);
}

hash_t *ZCachedName::get_file_hash()
{
	lite_assert(znr!=NULL);
	return znr->get_file_hash();
}

void ZCachedName::_update_status(ZCachedStatus new_status)
{
	if (new_status==zcs_dirty)
	{
		zcache->store_name(url_hint, znr);
		this->status = zcs_clean;
	}
	else
	{
		this->status = new_status;
	}
}
