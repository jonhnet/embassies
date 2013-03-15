#include "format_ip.h"
#include "AppLink.h"
#include "App.h"
#include "UnicastIfc.h"

UnicastIfc::UnicastIfc(AppLink *app_link)
{
	this->app_link = app_link;
}

void UnicastIfc::deliver(Packet *p)
{
	AppIfc key(*p->get_dest());
	AppIfc *app_ifc = app_link->lookup(key);
	if (app_ifc==NULL)
	{
		char dest[100];
		format_ip(dest, sizeof(dest), p->get_dest());
		fprintf(stderr, "dropping packet to nonexistent app %s\n", dest);
	}
	else
	{
#if DEBUG_EVERY_PACKET
		char app_ifc_s[100];
		format_ip(app_ifc_s, sizeof(app_ifc_s), app_ifc->dbg_get_addr());
		fprintf(stderr, "Unicast delivered to %s\n", app_ifc_s);
#endif // DEBUG_EVERY_PACKET
	
		app_ifc->get_app()->send_packet(p);
	}
	delete p;
}

XIPAddr UnicastIfc::get_subnet(XIPVer ver)
{
	return app_link->get_subnet(ver);
}

XIPAddr UnicastIfc::get_netmask(XIPVer ver)
{
	return app_link->get_netmask(ver);
}
