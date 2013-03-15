// deprecated -- its functions have all been factored into NavMgr
#if 0
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#include "SockMgr.h"
#include "NavigationProtocol.h"
#include "NavigationBlob.h"
#include "xax_extensions.h"

SockMgr::SockMgr(NavTest *nav_test)
	: _nav_test(nav_test)
{
	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(NavigationProtocol::PORT);
	addr.sin_addr.s_addr = INADDR_ANY;
	int rc = bind(sock_fd, (struct sockaddr*) &addr, sizeof(addr));
	lite_assert(rc==0);

	// Nonblocking lets us avoid having to integrate with poll;
	// we just loop until socket is dry.
	int flags = fcntl(sock_fd, F_GETFL, 0);
	rc = fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);
	lite_assert(rc==0);

	// Enable outgoing broadcasts. (LWIP probably just ignores this,
	// but we're trying to be a good citizen here.)
	int one = 1;
	rc = setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one));
	lite_assert(rc==0 || errno==ENOSYS);
}

void SockMgr::setup_poll(struct pollfd *pfd)
{
	pfd->fd = sock_fd;
	pfd->events = POLLIN;
	pfd->revents = 0;
}

void SockMgr::process_events()
{
	while (true)
	{
		uint8_t msg_buf[4096];
		int rc = read(sock_fd, msg_buf, sizeof(msg_buf));
		if (rc<0)
		{
			if (errno == EAGAIN)
			{
				// socket is dry; go back to main poll loop
				break;
			}
			perror("read");
			lite_assert(false);
		}
		lite_assert(rc > 0);
		lite_assert(rc <= (int) sizeof(msg_buf));
		
		NavigationBlob *msg = new NavigationBlob(msg_buf, rc);
		dispatch_msg(msg);
	}
}

void SockMgr::dispatch_msg(NavigationBlob *msg)
{
	uint32_t opcode;
	msg->read(&opcode, sizeof(opcode));
	switch (opcode)
	{
	case NavigateForward::OPCODE:
		fprintf(stderr, "Got NavigateForward\n");
		navigate_forward(msg);
		break;
	case NavigateBackward::OPCODE:
		fprintf(stderr, "Got NavigateBackward\n");
		navigate_backward(msg);
		break;
	case TMPaneHidden::OPCODE:
		fprintf(stderr, "Got PaneHidden\n");
		pane_hidden(msg);
		break;
	case TMPaneRevealed::OPCODE:
		fprintf(stderr, "Got PaneRevealed\n");
		pane_revealed(msg);
		break;
	case TMNewTabRequest::OPCODE:
		break;
	case TMNewTabReply::OPCODE:
		break;
	default:
		fprintf(stderr, "Ignoring nonsense opcode %d\n", opcode);
		break;
	}
	delete msg;
}

bool SockMgr::check_recipient_hack(hash_t *vendor_id_hash)
{
	hash_t me = _nav_test->get_vendor_id_hash();
	return memcmp(vendor_id_hash, &me, sizeof(hash_t))==0;
}

void SockMgr::navigate_forward(NavigationBlob *msg)
{
	NavigateForward nf;
	msg->read(&nf, sizeof(nf));
	if (!check_recipient_hack(&nf.dst_vendor_id_hash))
	{
		return;
	}
	NavigationBlob *ep = msg->read_blob();

	uint32_t ep_size = ep->size();
	const char *entry_point = (const char*) ep->read_in_place(ep_size);
	lite_assert(entry_point[ep_size-1] == '\0');
	fprintf(stderr, "NavTest received ep '%s'\n", entry_point);
	
	uint32_t color;
	color = atoi(entry_point);

	NavigationBlob *back_token = msg->read_blob();
	UIPane *uipane = new UIPane(_nav_test, nf.tab_deed, color, back_token);
	(void) uipane;	// stores itself in nav_test
}

void SockMgr::navigate_backward(NavigationBlob *msg)
{
	NavigateBackward nb;
	msg->read(&nb, sizeof(nb));
	if (!check_recipient_hack(&nb.dst_vendor_id_hash))
	{
		return;
	}
	NavigationBlob *bt = msg->read_blob();
	uint32_t color;
	bt->read(&color, sizeof(color));
	NavigationBlob *back_token = bt->read_blob();
	UIPane *uipane = new UIPane(_nav_test, nb.tab_deed, color, back_token);
	(void) uipane;	// stores itself in nav_test
}

void SockMgr::pane_hidden(NavigationBlob *msg)
{
	TMPaneHidden tmph;
	msg->read(&tmph, sizeof(tmph));
	UIPane *uipane = _nav_test->map_tab_deed_key_to_pane(tmph.tab_deed_key);
	if (uipane==NULL)
	{
		fprintf(stderr, "Failed to find matching tab_deed_key; must have been for someone else.\n");
		//lite_assert(false);
		return;
	}
	uipane->pane_hidden();
}

void SockMgr::pane_revealed(NavigationBlob *msg)
{
	TMPaneRevealed tmpr;
	msg->read(&tmpr, sizeof(tmpr));
	UIPane *uipane = _nav_test->map_tab_deed_key_to_pane(tmpr.tab_deed_key);
	if (uipane==NULL)
	{
		fprintf(stderr, "Faileed to find matching tab_deed_key; probably should ignore.\n");
		lite_assert(false);
		return;
	}
	uipane->pane_revealed(tmpr.pane_deed);
}
#endif
