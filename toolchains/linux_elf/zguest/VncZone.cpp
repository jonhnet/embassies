#include <assert.h>
#include "VncZone.h"
#include "RunConfig.h"
#include "AppIdentity.h"

VncZone::VncZone(XvncStartDetector *xvnc_start_detector)
	: Zone("vnc", 4, RunConfig::map_pie("xvnc"))
{
	this->xvnc_start_detector = xvnc_start_detector;

	char portargbuf[6];
	sprintf(portargbuf, ":%d", VNC_PORT_NUMBER);
	add_arg(portargbuf);
	add_arg("--PasswordFile=" ZOOG_ROOT "/toolchains/linux_elf/lib_links/vncpasswd_file");
	add_arg("-nolock");
	add_arg("-depth");
	add_arg("24");
	add_arg("-geometry");
	add_arg("1366x710");
}

void VncZone::verify_label()
{
	// get this done before vnc tries to accept_canvas
	AppIdentity app_identity;
	(get_zdt()->zoog_verify_label)(app_identity.get_cert_chain());
}

void VncZone::run()
{
	verify_label();
	Zone::run();
}

void VncZone::polymorphic_break()
{
}
