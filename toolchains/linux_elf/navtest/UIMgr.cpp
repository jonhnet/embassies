#include <unistd.h>
#include "UIMgr.h"
#include "xax_extensions.h"
#include "UIPane.h"
#include "NavTest.h"

UIMgr::UIMgr(NavTest *nav_test)
	: _nav_test(nav_test)
{
	ZoogHostAlarms *alarms = (_nav_test->get_zdt()->zoog_get_alarms)();
	ui_zutex_fd = xe_open_zutex_as_fd(&alarms->receive_ui_event);
}

void UIMgr::setup_poll(struct pollfd *pfd)
{
	pfd->fd = ui_zutex_fd;
	pfd->events = POLLIN;
	pfd->revents = 0;
}

void UIMgr::key_up(ZoogKeyEvent *evt)
{
	UIPane *pane = _nav_test->map_canvas_to_pane(evt->canvas_id);
	if (pane==NULL)
	{
		fprintf(stderr, "Dropping unloved keyevent (probably typed into a tab label)\n");
		return;
	}
	switch (evt->keysym)
	{
	case 'F': case 'f':
		pane->forward();
		break;
	case 'B': case 'b':
		pane->back();
		break;
	case 'T': case 't':
		pane->new_tab();
		break;
	default:
		fprintf(stderr, "Ignoring zoog keysym %d\n", evt->keysym);
	}
}

void UIMgr::process_events()
{
	ZoogUIEvent evt;

	// reset the poll evt first, then drain the queue,
	// to avoid a deadlock.
	char dummy;
	read(ui_zutex_fd, &dummy, 1);

	// then drain the queue, since we won't see another zutex bump until
	// a new item is *inserted* in the queue.
	while (true)
	{
		(_nav_test->get_zdt()->zoog_receive_ui_event)(&evt);
		switch (evt.type)
		{
		case zuie_no_event:
			return;
		case zuie_key_up:
			key_up(&evt.un.key_event);
			break;
		default:
			fprintf(stderr, "Ignoring zoog ui evt type %d\n", evt.type);
			break;
		}
	}
}

NavigateIfc *UIMgr::create_navigate_server()
{
	return new UIPane(_nav_test);
}
