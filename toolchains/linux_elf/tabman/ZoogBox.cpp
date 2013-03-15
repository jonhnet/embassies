#include <string.h>
#include "ZoogBox.h"
#include "XvncControlProtocol.h"
#include "Page.h"
#include "AppIdentity.h"
#include "NavMgr.h"

// important urls for gtk event mess:
// Thank you, http://stackoverflow.com/questions/2675514/totally-fed-up-with-get-gtk-widget-height-and-width
//	g_signal_connect(tab_eventbox, "size-allocate",
//		G_CALLBACK(populate_cb), this);
// or not. The size is allocated, but we have no math about offset
// from origin, because the widget isn't REALIZED or whatever.
// http://blog.yorba.org/jim/2010/10/those-realize-map-widget-signals.html


ZoogBox::ZoogBox(Page *page)
	: page(page),
	  map_mask(EMPTY_MASK),
	  eventbox(NULL),
	  landlord_viewport(0),
	  deed(0)
{
	eventbox = gtk_event_box_new();
	gtk_widget_show(eventbox);

	g_signal_connect(G_OBJECT(eventbox), "expose-event",
		G_CALLBACK(expose_cb), this);
	g_signal_connect(G_OBJECT(eventbox), "unmap",
		G_CALLBACK(unmap_cb), this);
}

void ZoogBox::expose_cb(GtkWidget *widget, GtkAllocation *allocation, void *data)
{
	((ZoogBox*) data)->mask_update(MASK_SET, EXPOSE_MASK);
}

void ZoogBox::unmap_cb(GtkWidget *widget, void *data)
{
	((ZoogBox*) data)->mask_update(MASK_CLEAR, EXPOSE_MASK);
}

bool ZoogBox::is_mapped()
{
	return (map_mask==mask_required());
}

void ZoogBox::mask_update(MaskChange change, MaskBit mask_bit)
{
	bool was = is_mapped();
	if (change==MASK_SET)
	{
		map_mask = (MaskBit) ( map_mask | mask_bit);
	}
	else if (change==MASK_CLEAR)
	{
		map_mask = (MaskBit) ( map_mask & ~mask_bit);
	}
	else
	{
		lite_assert(false);
	}
	bool is = is_mapped();

	if (!was && is)
	{
		map();
	}
	else if (was && !is)
	{
		unmap();
	}
}

void ZoogBox::map()
{
	// Get the dimensions of the widget
	ZRectangle rect;
	get_delagation_location(eventbox, &rect);

	// Get deed for viewport overlaid on widget
	XCClient xc;
	ViewportID tenant_viewport = xc.read_viewport();
	(page->get_zdt()->zoog_sublet_viewport)(
		tenant_viewport, &rect, &landlord_viewport, &deed);

	notify_tenant();
}

void ZoogBox::unmap()
{
}

void ZoogBox::send_msg(NavigationBlob *msg)
{
	NavSocket nsock;
	nsock.send_and_delete_msg(msg);
}

void ZoogBox::get_delagation_location(GtkWidget *container, ZRectangle *out_rect)
{
    GtkWidget* outer_container = gtk_widget_get_toplevel(container);
    if (!gtk_widget_is_toplevel(outer_container))
	{
		goto fail;
	}

	bool rc;
	gint x, y;
	rc = gtk_widget_translate_coordinates(container, outer_container, 0, 0, &x, &y);
	if (!rc)
	{
		goto fail;
	}
	out_rect->x = x;
	out_rect->y = y;
	out_rect->width = container->allocation.width;
	out_rect->height = container->allocation.height;
	return;	// success!

fail:
	lite_assert(false);
	out_rect->x = 0;
	out_rect->y = 0;
}

//////////////////////////////////////////////////////////////////////////////

TabBox::TabBox(Page *page)
	: ZoogBox(page),
	  pane_box(NULL)
{
	gtk_widget_set_size_request(eventbox, 80, 24);
}

void TabBox::set_pane_box(PaneBox *pane_box)
{
	this->pane_box = pane_box;
	if (is_mapped())
	{
		pane_box->mask_update(MASK_SET, TAB_PRESENT_MASK);
	}
}

void TabBox::map()
{
	ZoogBox::map();
	// Pane may have gotten its first_expose before we did, and
	// had to wait. Give it another shot.
	if (pane_box!=NULL)
	{
		pane_box->mask_update(MASK_SET, TAB_PRESENT_MASK);
	}
}

void TabBox::notify_tenant()
{
	fprintf(stderr, "TabBox %p sends NavigateForward\n", this);
	NavigationBlob *msg = new NavigationBlob();
	uint32_t opcode = NavigateForward::OPCODE;
	msg->append(&opcode, sizeof(opcode));
	AppIdentity ai("results.search.com");
//	AppIdentity ai("vendor-a");

	NavigateForward nf;
	nf.dst_vendor_id_hash = ai.get_pub_key_hash();
	nf.rpc_nonce = 0;	// not expecting a reply...
	nf.tab_deed = deed;
	msg->append(&nf, sizeof(nf));

	NavigationBlob entry_point_blob(4);
	char buf[200];
	snprintf(buf, sizeof(buf), "%d.html", page->get_coloridx());
	entry_point_blob.append(buf, strlen(buf)+1);
	msg->append(&entry_point_blob);

	NavigationBlob null_back_blob(0);
	msg->append(&null_back_blob);
	send_msg(msg);
}

DeedKey TabBox::get_deed_key()
{
	DeedKey result;
	(page->get_zdt()->zoog_get_deed_key)(get_landlord_viewport(), &result);
	return result;
}

//////////////////////////////////////////////////////////////////////////////

PaneBox::PaneBox(Page *page, TabBox *tab_box)
	: ZoogBox(page),
	  tab_box(tab_box)
{
	gtk_widget_set_size_request(eventbox, 580, 500);
	mask_update(MASK_SET, TAB_NOT_TRANSFERRING);
}

void PaneBox::map()
{
	fprintf(stderr, "PaneBox::expose()\n");
	ZoogBox::map();
}

void PaneBox::notify_tenant()
{
	fprintf(stderr, "PaneBox %p sends TMPaneRevealed\n", this);
	NavigationBlob *msg = new NavigationBlob();
	uint32_t opcode = TMPaneRevealed::OPCODE;
	msg->append(&opcode, sizeof(opcode));

	TMPaneRevealed tmpr;
	// "route" message to correct tab implementation using tab's DeedKey
	(page->get_zdt()->zoog_get_deed_key)(tab_box->get_landlord_viewport(), &tmpr.tab_deed_key);
	tmpr.pane_deed = deed;
	msg->append(&tmpr, sizeof(tmpr));

	send_msg(msg);
}

void PaneBox::unmap()
{
	fprintf(stderr, "PaneBox %p sends TMPaneHidden\n", this);

	NavigationBlob *msg = new NavigationBlob();
	uint32_t opcode = TMPaneHidden::OPCODE;
	msg->append(&opcode, sizeof(opcode));

	TMPaneHidden tmph;
	// "route" message to correct tab implementation using tab's DeedKey
	(page->get_zdt()->zoog_get_deed_key)(tab_box->get_landlord_viewport(), &tmph.tab_deed_key);
	msg->append(&tmph, sizeof(tmph));

	send_msg(msg);

	// for debugging convenience
	this->landlord_viewport = 0;
	this->deed = 0;
}

void PaneBox::switch_page(bool enter)
{
	mask_update(enter ? MASK_SET : MASK_CLEAR, SWITCH_PAGE_MASK);
}

void PaneBox::transfer(bool transferring)
{
	mask_update(
		transferring ? MASK_CLEAR : MASK_SET,
		TAB_NOT_TRANSFERRING);
}
