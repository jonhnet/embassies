#include "AppLink.h"
#include "App.h"
#include "BroadcastIfc.h"
#include "format_ip.h"

BroadcastIfc::BroadcastIfc(AppLink *app_link, TunnelIfc *tunnel_ifc, BroadcastVariant variant)
{
	this->app_link = app_link;
	this->tunnel_ifc = tunnel_ifc;
	this->variant = variant;
}

const char *BroadcastIfc::get_name()
{
	switch (variant)
	{
	case ONES:	return "broadcast-ff";
	case LINK:	return "broadcast-link";
	}
	lite_assert(false);
}

void BroadcastIfc::_deliver_one(void *v_packet, void *v_app_ifc)
{
	Packet *p = (Packet *) v_packet;
	AppIfc *app_ifc = (AppIfc *) v_app_ifc;

	if (app_ifc->get_version() != p->get_dest()->version)
	{
		// deliver broadcast to corresponding IP-type net ifc, to avoid
		// multiple deliveries to same app. (Just confusing for debugging.)
		return;
	}

#if DEBUG_EVERY_PACKET
	char bcast_addr_s[100], app_ifc_s[100];
	format_ip(bcast_addr_s, sizeof(bcast_addr_s), p->get_dest());
	format_ip(app_ifc_s, sizeof(app_ifc_s), app_ifc->dbg_get_addr());
	fprintf(stderr, "Broadcast %s delivered to %s\n", bcast_addr_s, app_ifc_s);
#endif // DEBUG_EVERY_PACKET

	app_ifc->get_app()->send_cm(&p->get_as_cm()->hdr, p->get_cm_size());
}

void BroadcastIfc::deliver(Packet *p)
{
//	app_link->dbg_dump_table();
	app_link->visit_every(_deliver_one, p);
	if (p->packet_came_from_tunnel())
	{
		// don't deliver back to tunnel, then
		delete p;
	}
	else
	{
		tunnel_ifc->deliver(p);
		// tunnel_ifc assumes ownership of p
	}
}

XIPAddr BroadcastIfc::get_subnet(XIPVer ver)
{
	switch (variant)
	{
	case ONES:	return xip_all_ones(ver);
	case LINK:	return app_link->get_broadcast(ver);
	}
	lite_assert(false);
	XIPAddr dummy; return dummy;
}

XIPAddr BroadcastIfc::get_netmask(XIPVer ver)
{
	switch (variant)
	{
	case ONES:	return xip_all_ones(ver);
	case LINK:	return xip_all_ones(ver);
	}
	lite_assert(false);
	XIPAddr dummy; return dummy;
}
