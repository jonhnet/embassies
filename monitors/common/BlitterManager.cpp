#include "BlitterManager.h"
#include "BlitterViewport.h"
#include "BlitterCanvas.h"
#include "LiteLib.h"
#include "DeedEntry.h"
#include "ZRectangle.h"

BlitterManager::BlitterManager(BlitProviderIfc *provider_, MallocFactory *mf, SyncFactory *sf, RandomSupply *random_supply)
	: provider(provider_),
	  mf(mf),
	  random_supply(random_supply),
	  next_canvas_id(1),
	  dbg_deeds_log(mf),
	  dbg_canvases_log(mf)
{
	linked_list_init(&palifcs, mf);
	hash_table_init(&viewport_handles, mf, ViewportHandle::hash, ViewportHandle::cmp);
	hash_table_init(&deeds, mf, DeedEntry::hash, DeedEntry::cmp);
	hash_table_init(&canvases, mf, BlitterCanvas::hash, BlitterCanvas::cmp);
	mutex = sf->new_mutex(true);
}

BlitterManager::~BlitterManager()
{ }

void BlitterManager::register_palifc(BlitPalIfc *palifc)
{
	mutex->lock();
	linked_list_insert_tail(&palifcs, palifc);
	mutex->unlock();
}

	class CanvasCleanerUpper
	{
	private:
		BlitPalIfc *target_palifc;
		LinkedList target_canvas_ids;

		static void static_visitor(void *uo, void *datum);
		void visit(BlitterCanvas *canvas);

	public:
		CanvasCleanerUpper(
			BlitterManager *blitter_mgr, BlitPalIfc *target_palifc);
	};

	CanvasCleanerUpper::CanvasCleanerUpper(
		BlitterManager *blitter_mgr, BlitPalIfc *target_palifc)
	{
		this->target_palifc = target_palifc;
		linked_list_init(&target_canvas_ids, blitter_mgr->get_mf());

		hash_table_visit_every(
			&blitter_mgr->canvases, static_visitor, this);

		while (target_canvas_ids.count>0)
		{
			ZCanvasID *cptr =
				(ZCanvasID*) linked_list_remove_head(&target_canvas_ids);
			ZCanvasID canvas_id = *cptr;
			delete cptr;
			BlitterCanvas *canvas = blitter_mgr->get_canvas(canvas_id);
			if (canvas!=NULL)
			{
				canvas->unmap();
			}
		}
		// TODO should probably tidy up tree of viewports, not canvases
	}

	void CanvasCleanerUpper::static_visitor(void *uo, void *datum)
	{
		((CanvasCleanerUpper *) uo)->visit((BlitterCanvas *) datum);
	}

	void CanvasCleanerUpper::visit(BlitterCanvas *canvas)
	{
		if (canvas->get_palifc()==target_palifc)
		{
			ZCanvasID *cid = new ZCanvasID;
			*cid = canvas->get_canvas_id();
			linked_list_insert_tail(&target_canvas_ids, cid);
		}
	}

void BlitterManager::unregister_palifc(BlitPalIfc *palifc)
{
	mutex->lock();
	// Avoid trying to multicast events out to this now-defunct palifc
	linked_list_remove(&palifcs, palifc);

	// We also need to kill off any canvas attached to the expired
	// palifc, to avoid routing any messages to it.
	// We track these by id, then look them up, because the removal of
	// one may automagically cascade to remove others (via nested viewports).
	CanvasCleanerUpper cleaner_upper(this, palifc);
	mutex->unlock();
}

void BlitterManager::multicast_ui_event(ZPubKey *vendor, ZoogUIEvent *evt)
{
	mutex->lock();
	LinkedListIterator lli;
	for (ll_start(&palifcs, &lli); ll_has_more(&lli); ll_advance(&lli))
	{
		BlitPalIfc *palifc = (BlitPalIfc *) ll_read(&lli);
		if (palifc->get_pub_key()->isEqual(vendor))
		{
			palifc->get_uidelivery()->deliver_ui_event(evt);
		}
	}
	mutex->unlock();
}

void BlitterManager::add_viewport_handle(ViewportHandle *viewport_handle)
{
	mutex->lock();
	hash_table_insert(&viewport_handles, viewport_handle);
	mutex->unlock();
}

void BlitterManager::remove_viewport_handle(ViewportHandle *viewport_handle)
{
	mutex->lock();
	hash_table_remove(&viewport_handles, viewport_handle);
	mutex->unlock();
}

BlitterViewport *BlitterManager::get_viewport(ViewportID viewport_id, uint32_t process_id)
{
	// NB this lock is no guarantee the viewport object will remain alive
	// after you're done...
	mutex->lock();
	ViewportHandle key(viewport_id, process_id, NULL);
	ViewportHandle *vh = (ViewportHandle *) hash_table_lookup(&viewport_handles, &key);
	BlitterViewport *result = vh->get_viewport();
	mutex->unlock();
	return result;
}

BlitterViewport *BlitterManager::get_tenant_viewport(ViewportID viewport_id, uint32_t process_id)
{
	mutex->lock();
	BlitterViewport *viewport = get_viewport(viewport_id, process_id);
	if (viewport!=NULL && viewport->get_tenant_id() != viewport_id)
	{
		viewport = NULL;
	}
	mutex->unlock();
	return viewport;
}

BlitterViewport *BlitterManager::get_landlord_viewport(ViewportID viewport_id, uint32_t process_id)
{
	mutex->lock();
	BlitterViewport *viewport = get_viewport(viewport_id, process_id);
	if (viewport!=NULL && viewport->get_landlord_id() != viewport_id)
	{
		viewport = NULL;
	}
	mutex->unlock();
	return viewport;
}

BlitterViewport* BlitterManager::accept_viewport(Deed deed, uint32_t process_id)
{
	mutex->lock();
	BlitterViewport* result;
	DeedEntry key(deed, NULL);
	DeedEntry *value = (DeedEntry*) hash_table_lookup(&deeds, &key);
	if (value==NULL)
	{
		result = NULL;
	}
	else
	{
		hash_table_remove(&deeds, value);
		BlitterViewport *viewport = value->get_viewport();
		delete value;
		viewport->accept_viewport(process_id);
		result = viewport;
	}

	{
		char buf[200];
		snprintf(buf, sizeof(buf), "%016llx deed accepted\n", deed);
		dbg_deeds_log.append(buf);
	}
	mutex->unlock();

	return result;
}

void BlitterManager::add_canvas(BlitterCanvas *canvas)
{
	mutex->lock();
	{
		char buf[200];
		snprintf(buf, sizeof(buf), "add_canvas(%p [%d])\n", canvas, canvas->get_canvas_id());
		dbg_canvases_log.append(buf);
	}
	hash_table_insert(&canvases, canvas);
	mutex->unlock();
}

void BlitterManager::remove_canvas(BlitterCanvas *canvas)
{
	mutex->lock();
	{
		char buf[200];
		snprintf(buf, sizeof(buf), "remove_canvas(%p [%d])\n", canvas, canvas->get_canvas_id());
		dbg_canvases_log.append(buf);
	}
	hash_table_remove(&canvases, canvas);
	mutex->unlock();
}

BlitterCanvas *BlitterManager::get_canvas(ZCanvasID canvas_id)
{
	// NB no guarantee canvas stays alive after lock, though...
	mutex->lock();
	BlitterCanvas key(canvas_id);
	BlitterCanvas *result = (BlitterCanvas *) hash_table_lookup(&canvases, &key);
	mutex->unlock();
	return result;
}

ViewportID BlitterManager::new_toplevel_viewport(uint32_t process_id)
{
	mutex->lock();
	ZRectangle default_toplevel_rect = NewZRectangle(50, 50, 700, 700);
	BlitterViewport *bv = new BlitterViewport(
		this, process_id, NULL, &default_toplevel_rect);
	bv->accept_viewport(process_id);
	ViewportID result = bv->get_tenant_id();
	mutex->unlock();
	return result;
}

ZCanvasID BlitterManager::allocate_canvas_id()
{
	mutex->lock();
	ZCanvasID result = next_canvas_id;
	next_canvas_id+=1;
	mutex->unlock();
	return result;
}

void BlitterManager::manufacture_deed(BlitterViewport *viewport, Deed *out_deed, DeedKey *out_deed_key)
{
	mutex->lock();
	random_supply->get_random_bytes((uint8_t*) out_deed, sizeof(*out_deed));

	{
		char buf[200];
		snprintf(buf, sizeof(buf), "%016llx via supply %p\n", *out_deed, random_supply);
		dbg_deeds_log.append(buf);
	}

	random_supply->get_random_bytes((uint8_t*) out_deed_key, sizeof(*out_deed_key));
	DeedEntry *entry = new DeedEntry(*out_deed, viewport);
	hash_table_insert(&deeds, entry);
	mutex->unlock();
}
