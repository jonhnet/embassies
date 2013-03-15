#include "pal_abi/pal_abi.h"
#include "pal_abi/pal_extensions.h"
#include "simple_network.h"
#include "xax_network_utils.h"
#include "cheesy_snprintf.h"
#include "format_ip.h"

#include "relocate_this.h"

ZoogDispatchTable_v1 *g_zdt = 0;
debug_logfile_append_f *g_debuglog = 0;


void zoog_sleep_ms(ZoogDispatchTable_v1 *zdt, uint32_t time_ms)
{
	ZoogHostAlarms *alarms = (zdt->zoog_get_alarms)();
	uint64_t target_time;
	zdt->zoog_get_time(&target_time);
	target_time += ((uint64_t) time_ms)*1000000;

	while (true)
	{
		uint64_t now;
		zdt->zoog_get_time(&now);
		if (now >= target_time) { break; }

		uint32_t match_val = alarms->clock;
		(zdt->zoog_set_clock_alarm)(target_time);
		ZutexWaitSpec spec = { &alarms->clock, match_val };
		(zdt->zoog_zutex_wait)(&spec, 1);
	}
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
	(g_debuglog)("stderr", "LAUNCHEE RAN! Wooo!\n");

	XIPifconfig ifconfig[2];
	int ifcount = 2;
	(g_zdt->zoog_get_ifconfig)(ifconfig, &ifcount);
	lite_assert(ifcount==2);

	UDPEndpoint source_ep, dest_ep;
	source_ep.ipaddr = ifconfig[1].local_interface;
	source_ep.port = z_htons(16);
	dest_ep.ipaddr = get_ip_subnet_broadcast_address(ipv6);
	dest_ep.port = z_htons(16);

	char ipbuf[100], buf[1000];
	format_ip(ipbuf, sizeof(ipbuf), &dest_ep.ipaddr);
	cheesy_snprintf(buf, sizeof(buf), "broadcasting to %s\n", ipbuf);
	(g_debuglog)("stderr", buf);

	int i;
	for (i=30; i<=90; i++)
	{
		// Wait for 0.5 sec
		zoog_sleep_ms(zdt, 500);

		// Then send a ping.
		char buf[1000];
		char ipbuf[100];
		format_ip(ipbuf, sizeof(ipbuf), &source_ep.ipaddr);
		cheesy_snprintf(buf, sizeof(buf), "source %s\n", ipbuf);
		(g_debuglog)("stderr", buf);
		format_ip(ipbuf, sizeof(ipbuf), &dest_ep.ipaddr);
		cheesy_snprintf(buf, sizeof(buf), "dest %s\n", ipbuf);
		(g_debuglog)("stderr", buf);

		{
//			uint32_t payload_size = (i<23) ? 1<<i : 1<<26;
//			uint32_t payload_size = 1<<9;
			uint32_t payload_size = 512;
			void *v_payload;
			ZoogNetBuffer *znb = create_udp_packet_xnb(g_zdt, source_ep, dest_ep, payload_size, &v_payload);
			uint32_t *payload = (uint32_t*) v_payload;
			(*payload) = 16+i;

			(g_zdt->zoog_send_net_buffer)(znb, true);
		}
		(g_debuglog)("stderr", "ping end\n");
	}

	(zdt->zoog_exit)();
}
