#include <assert.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "XvncControlProtocol.h"

XCClient::XCClient()
	: used(false)
{
	pipesock = socket(AF_UNIX, SOCK_STREAM, 0);
	assert(pipesock > 0);

	struct sockaddr_un sun;
	sun.sun_family = AF_UNIX;

	strcpy(sun.sun_path, XVNC_CONTROL_SOCKET_PATH);
	int rc;
	rc = connect(pipesock, (struct sockaddr*) &sun, sizeof(sun));
	assert(rc==0);
}

XCClient::~XCClient()
{
	close(pipesock);
}

void XCClient::use()
{
	assert(!used);
	used = true;
}

void XCClient::change_viewport(ViewportID viewport_id)
{
	use();
	int rc;
	XCChangeViewport xc_change_viewport;
	xc_change_viewport.opcode = XCChangeViewport::OPCODE;
	xc_change_viewport.viewport_id = viewport_id;
	rc = write(pipesock, &xc_change_viewport, sizeof(xc_change_viewport));
	assert(rc==sizeof(xc_change_viewport));
}

void XCClient::unmap_canvas()
{
	use();
	int rc;
	XCUnmapCanvas xc_unmap_canvas;
	xc_unmap_canvas.opcode = XCUnmapCanvas::OPCODE;
	rc = write(pipesock, &xc_unmap_canvas, sizeof(xc_unmap_canvas));
	assert(rc==sizeof(xc_unmap_canvas));
}

ViewportID XCClient::read_viewport()
{
	use();
	int rc;
	XCReadViewportRequest xc_read_viewport_request;
	xc_read_viewport_request.opcode = XCReadViewportRequest::OPCODE;
	rc = write(pipesock, &xc_read_viewport_request, sizeof(xc_read_viewport_request));
	assert(rc==sizeof(xc_read_viewport_request));

	XCReadViewportReply xc_read_viewport_reply;
	rc = read(pipesock, &xc_read_viewport_reply, sizeof(xc_read_viewport_reply));
	assert(rc==sizeof(xc_read_viewport_reply));

	return xc_read_viewport_reply.viewport_id;
}
