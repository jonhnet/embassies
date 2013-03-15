#include <poll.h>
#include <unistd.h>

#include "xax_extensions.h"

#include "LiteLib.h"
#include "NavTest.h"
#include "UIMgr.h"
//#include "SockMgr.h"
#include "ambient_malloc.h"
#include "standard_malloc_factory.h"
#include "AppIdentity.h"
#include "NavigationProtocol.h" 
#include "UrlEP.h" 
#include "NavMgr.h" 

NavTest::NavTest()
{
	_zdt = xe_get_dispatch_table();
	_verify_label();
	linked_list_init(&panes, standard_malloc_factory_init());
	_run();
}

void NavTest::_verify_label()
{
	AppIdentity app_identity;
	ZCertChain *chain = app_identity.get_cert_chain();
	(_zdt->zoog_verify_label)(chain);
}

void NavTest::_run()
{
	UIMgr uimgr(this);
	UrlEPFactory url_factory;
	NavMgr navmgr(&uimgr, &url_factory);
//	SockMgr sockmgr(this);
	while (1)
	{
		enum { ui_idx, /*sock_idx,*/ num_idx };
		struct pollfd fds[num_idx];
		uimgr.setup_poll(&fds[ui_idx]);
//		sockmgr.setup_poll(&fds[sock_idx]);
		int rc = poll(fds, num_idx, -1);
		lite_assert(rc>0);

/*
		if (fds[sock_idx].revents!=0)
		{
			sockmgr.process_events();
		}
*/
		if (fds[ui_idx].revents!=0)
		{
			uimgr.process_events();
		}
	}
}

ZoogDispatchTable_v1 *NavTest::get_zdt()
{
	return _zdt;
}

void NavTest::add_pane(UIPane *uipane)
{
	linked_list_insert_head(&panes, uipane);
}

void NavTest::remove_pane(UIPane *uipane)
{
	linked_list_remove(&panes, uipane);
}

UIPane *NavTest::map_canvas_to_pane(ZCanvasID canvas_id)
{
	LinkedListIterator lli;
	for (ll_start(&panes, &lli); ll_has_more(&lli); ll_advance(&lli))
	{
		UIPane *pane = (UIPane*) ll_read(&lli);
		if (pane->has_canvas(canvas_id))
		{
			return pane;
		}
	}
	return NULL;
}

int main(int argc, char **argv)
{
	ambient_malloc_init(standard_malloc_factory_init());
	NavTest nav_test;
	return 0;
}
