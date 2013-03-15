#include "AppLink.h"
#include "App.h"
#include "format_ip.h"

AppLink::AppLink(SyncFactory *sf, MallocFactory *mf, TunIDAllocator* tunid_a)
{
/*
	hash_table_init(&app_ifc_table, mf, AppIfc::hash, AppIfc::cmp);
	hash_table_init(&stale_ifcs, mf, AppIfc::hash, AppIfc::cmp);
*/
	app_ifc_tree = new AppIfcTree(sf);

	int tunid = tunid_a->get_tunid();

	char buf[200];
	snprintf(buf, sizeof(buf), "10.%d.0.0", tunid);
	subnet[XIPVer_to_idx(ipv4)] = decode_ipv4addr_assertsuccess(buf);
	snprintf(buf, sizeof(buf), "10.%d.0.1", tunid);
	gateway[XIPVer_to_idx(ipv4)] = decode_ipv4addr_assertsuccess(buf);
	snprintf(buf, sizeof(buf), "255.255.0.0");
	netmask[XIPVer_to_idx(ipv4)] = decode_ipv4addr_assertsuccess(buf);

	snprintf(buf, sizeof(buf), "fe80::0a:%x:0:0", tunid);
	subnet[XIPVer_to_idx(ipv6)] = decode_ipv6addr_assertsuccess(buf);
	snprintf(buf, sizeof(buf), "fe80::0a:%x:0:1", tunid);
	gateway[XIPVer_to_idx(ipv6)] = decode_ipv6addr_assertsuccess(buf);
	netmask[XIPVer_to_idx(ipv6)] = decode_ipv6addr_assertsuccess(
		"ffff:ffff:ffff:ffff:ffff:ffff:ffff:ff00");
}

AppLink::~AppLink()
{
//	lite_assert(app_ifc_table.count==0);
	lite_assert(app_ifc_tree->size()==0);
	// TODO implement teardown
	delete app_ifc_tree;
}

void AppLink::connect(App *app)
{
	for (int i=0; i<2; i++)
	{
		AppIfc *app_ifc = new AppIfc(*app->get_ip_addr(idx_to_XIPVer(i)), app);
//		hash_table_insert(&app_ifc_table, app_ifc);
		app_ifc_tree->insert(app_ifc);
	}
	dbg_dump_table();
}

void AppLink::disconnect(App *app)
{
	dbg_dump_table();
	for (int i=0; i<2; i++)
	{
		AppIfc key(*app->get_ip_addr(idx_to_XIPVer(i)));
//		AppIfc *app_ifc = (AppIfc*) hash_table_lookup(&app_ifc_table, &key);
		AppIfc *app_ifc = app_ifc_tree->lookup(key);
		lite_assert(app_ifc!=NULL);
		app_ifc_tree->remove(app_ifc);
//		hash_table_insert(&stale_ifcs, app_ifc);
	}
}

#if 0
void AppLink::_tidy_one(void *v_this, void *v_app_ifc)
{
	AppLink *app_link = (AppLink *) v_this;
	AppIfc *app_ifc = (AppIfc *) v_app_ifc;
	hash_table_remove(&app_link->app_ifc_table, app_ifc);
	delete app_ifc;
}

void AppLink::_tidy()
{
	hash_table_remove_every(&stale_ifcs, _tidy_one, this);
}

HashTable *AppLink::get_app_ifc_table()
{
//	_tidy();
	return &app_ifc_table;
}
#endif

void AppLink::visit_every(ForeachFunc *ff, void *user_obj)
{
//	hash_table_visit_every(&app_ifc_table, ff, user_obj);
	AppIfc *elt = app_ifc_tree->findMin();
	while (elt!=NULL)
	{
		AppIfc key = elt->getKey();
			// NB we copy elt key into a private copy, because (ff)
			// may delete the elt itself, and we want to be able to continue
			// iterating.
		(ff)(user_obj, elt);
		elt = app_ifc_tree->findFirstGreaterThan(key);
	}
}

AppIfc *AppLink::lookup(AppIfc key)
{
//	return hash_table_lookup(&app_ifc_table, &key);
	return app_ifc_tree->lookup(key);
}

XIPAddr AppLink::get_subnet(XIPVer ver)
{
	return subnet[XIPVer_to_idx(ver)];
}

XIPAddr AppLink::get_netmask(XIPVer ver)
{
	return netmask[XIPVer_to_idx(ver)];
}

XIPAddr AppLink::get_gateway(XIPVer ver)
{
	return gateway[XIPVer_to_idx(ver)];
}

XIPAddr AppLink::get_broadcast(XIPVer ver)
{
	return xip_broadcast(
		&subnet[XIPVer_to_idx(ver)], &netmask[XIPVer_to_idx(ver)]);
}

void AppLink::_dbg_dump_one(void *v_this, void *v_app_ifc)
{
	AppIfc *app_ifc = (AppIfc *) v_app_ifc;
	char buf[200];
	format_ip(buf, sizeof(buf), app_ifc->dbg_get_addr());
	fprintf(stderr, "  AppIfc at %s\n", buf);
}

void AppLink::dbg_dump_table()
{
	fprintf(stderr, "AppLink connections:\n");
	visit_every(_dbg_dump_one, this);
}

