#include "pal_abi/pal_abi.h"
#include "pal_abi/pal_extensions.h"
#include "xax_network_defs.h"
#include "xax_network_utils.h"
#include "xax_util.h"
#include "LiteLib.h"
#include "cheesy_snprintf.h"

#include "relocate_this.h"
#include "launchee_test.signed.h"

ZoogDispatchTable_v1 *g_zdt = 0;
debug_logfile_append_f *g_debuglog = 0;

ZoogNetBuffer *blocking_receive(ZoogDispatchTable_v1 *zdt, ZoogHostAlarms *alarms)
{
	ZoogNetBuffer *xnb;
	while (1)
	{
		uint32_t match_val = alarms->receive_net_buffer;
		xnb = (zdt->zoog_receive_net_buffer)();
		if (xnb!=NULL) { break; }

		char buf[1000];
		cheesy_snprintf(buf, sizeof(buf), "znb=NULL; waiting on alarm==%d\n",
			match_val);
		(g_debuglog)("stderr", buf);

		ZutexWaitSpec spec = { &alarms->receive_net_buffer, match_val };
		(zdt->zoog_zutex_wait)(&spec, 1);
	}
	return xnb;
}

void
elfmain(
	ZoogDispatchTable_v1 *zdt,
	uint32_t elfbase,
	uint32_t rel_offset,
	uint32_t rel_count)
{
	// First things first: fix up relocs so we can use globals & func ptrs
	// and pretty much everything else.
	relocate_this(elfbase, rel_offset, rel_count);

	g_zdt = zdt;

	g_debuglog = (zdt->zoog_lookup_extension)("debug_logfile_append");
	(g_debuglog)("stderr", "Beginning launcher test.\n");
	SignedBinary *signed_binary = (SignedBinary*) launchee_test_signed;

	(zdt->zoog_launch_application)(signed_binary);
	(g_debuglog)("stderr", "Launcher has launched.\n");
	ZoogHostAlarms *alarms = (zdt->zoog_get_alarms)();

	int tick=0;
	while (1)
	{
	ZoogNetBuffer *xnb = blocking_receive(zdt, alarms);

		IPInfo info;
		decode_ip_packet(&info, &xnb[1], xnb->capacity);
		UDPPacket *udpp = (UDPPacket*) (((void*)&xnb[1])+info.payload_offset);
		uint32_t *payload = (uint32_t *) &udpp[1];

		int dest_port = z_ntohs(udpp->dest_port);

		if (dest_port == UDP_PORT_FAKE_TIMER)
		{
			tick++;
			if ((tick % 10)==0)
			{
				(g_debuglog)("stderr", "tick\n");
			}
		}
		else
		{
			char buf[1000];
			cheesy_snprintf(buf, sizeof(buf),
				"packet to udp port %d message %d size %x XNB capacity %x\n",
				dest_port, *payload, info.packet_length, xnb->capacity);
			(g_debuglog)("stderr", buf);
		}

		(g_zdt->zoog_free_net_buffer)(xnb);
	}

	(zdt->zoog_exit)();
}
