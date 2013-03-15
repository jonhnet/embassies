#include "LiteLib.h"
#include "xax_network_utils.h"
#include "Router.h"
#include "ForwardingEntry.h"
#include "BroadcastIfc.h"
#include "UnicastIfc.h"

Router::Router(SyncFactory *sf, MallocFactory *mf, TunIDAllocator* tunid)
{
	linked_list_init(&forwarding_table, mf);

	// must call open_tunnel() first, first, since it's the one that assigns
	// a tunid if the user doesn't have one.
	tunid->open_tunnel();

	// But TunnelIfc wants AppLink initted first, since AppLink
	// constructs ip addrs.
	app_link = new AppLink(sf, mf, tunid);

	tunnel_ifc = new TunnelIfc(this, app_link, tunid);

	_create_forwarding_table();
}
	
void Router::_create_forwarding_table()
{
#if 0
	XIPAddr zero4 = decode_ipv4addr_assertsuccess("0.0.0.0");
	XIPAddr zero6 = decode_ipv6addr_assertsuccess("::");
	XIPAddr one4 = decode_ipv4addr_assertsuccess("255.255.255.255");
	XIPAddr one6 = decode_ipv6addr_assertsuccess("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
#endif
	
	// routes entered from most generic to most specific

	// default route
	forwarding_table_add(tunnel_ifc);

	// link-wide route (to apps)
	forwarding_table_add(new UnicastIfc(app_link));

	// link broadcast
	forwarding_table_add(new BroadcastIfc(app_link, tunnel_ifc, BroadcastIfc::ONES));
	forwarding_table_add(new BroadcastIfc(app_link, tunnel_ifc, BroadcastIfc::LINK));

	// gateway, specifically.
	// (needed since netmask matches on link otherwise hit deliver_local)
	for (int i=0; i<2; i++)
	{
		XIPVer ver = idx_to_XIPVer(i);
		ForwardingEntry *fe = new ForwardingEntry(tunnel_ifc, app_link->get_gateway(ver), xip_all_ones(ver), "gateway");
		linked_list_insert_head(&forwarding_table, fe);
	}

	dbg_forwarding_table_print();
}

void Router::forwarding_table_add(NetIfc *ni)
{
	for (int i=0; i<2; i++)
	{
		XIPVer ver = idx_to_XIPVer(i);
		ForwardingEntry *fe = new ForwardingEntry(ni, ni->get_subnet(ver), ni->get_netmask(ver), ni->get_name());
		linked_list_insert_head(&forwarding_table, fe);
	}
}

void Router::deliver_packet(Packet *p)
{
	if (!p->valid())
	{
		// this packet spewed a safety violation at construction;
		// ignore it here.
		return;
	}
	NetIfc *net_ifc = lookup(p->get_dest());
	net_ifc->deliver(p);
}

void Router::connect_app(App *app)
{
	app_link->connect(app);
}

void Router::disconnect_app(App *app)
{
	app_link->disconnect(app);
}

NetIfc *Router::lookup(XIPAddr *addr)
{
	LinkedListIterator li;
	for (ll_start(&forwarding_table, &li); ll_has_more(&li); ll_advance(&li))
	{
		ForwardingEntry *fe = (ForwardingEntry *) ll_read(&li);
		if (fe->mask_match(addr))
		{
			return fe->get_net_ifc();
		}
	}
	lite_assert(false);	// expected entry absent
	return NULL;
}

XIPAddr Router::get_gateway(XIPVer ver)
{
	return app_link->get_gateway(ver);
}

XIPAddr Router::get_netmask(XIPVer ver)
{
	return app_link->get_netmask(ver);
}

void Router::dbg_forwarding_table_print()
{
	fprintf(stderr, "Forwarding table:\n");
	LinkedListIterator li;
	for (ll_start(&forwarding_table, &li);
		ll_has_more(&li);
		ll_advance(&li))
	{
		ForwardingEntry *fe = (ForwardingEntry *) ll_read(&li);
		char buf[300];
		fe->print(buf, sizeof(buf));
		fprintf(stderr, "  %s\n", buf);
	}
}

