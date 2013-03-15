#include "paltest.h"
#include "simple_network.h"
#include "xax_network_utils.h"
#include "cheesy_snprintf.h"

void msg(char *m)
{
	(g_debuglog)("stderr", m);
}

void recvtest()
{
	XIPifconfig ifconfigs[2];
	int count = 2;
	(g_zdt->zoog_get_ifconfig)(ifconfigs, &count);

	ZoogHostAlarms *zha = (g_zdt->zoog_get_alarms)();
	char buf[1024];

	while (true)
	{
		ZoogNetBuffer *znb;
		while (true)
		{
			msg("calling receive_net_buffer()\n");
			uint32_t receive_net_buffer_value = zha->receive_net_buffer;

			znb = (g_zdt->zoog_receive_net_buffer)();
			if (znb!=NULL)
			{
				break;
			}

			cheesy_snprintf(buf, sizeof(buf),
				"receive_net_buffer() returns NULL; waiting on zutex %d\n",
				receive_net_buffer_value);
			msg(buf);
			ZutexWaitSpec spec;
			spec.zutex = &zha->receive_net_buffer;
			spec.match_val = receive_net_buffer_value;
			(g_zdt->zoog_zutex_wait)(&spec, 1);
		}

		char ipdesc[1024];
		IPInfo info;
		decode_ip_packet(&info, ZNB_DATA(znb), znb->capacity);
		if (info.packet_valid)
		{
			cheesy_snprintf(ipdesc, sizeof(ipdesc),
				"ip len %d, dest %08x",
				info.packet_length,
				info.dest.xip_addr_un.xv4_addr);
		}
		else
		{
			lite_strcpy(ipdesc, "invalid IP");
		}
		cheesy_snprintf(buf, sizeof(buf), "receive_net_buffer() returned %d-capacity buffer; info %s\n", znb->capacity, ipdesc);
		msg(buf);

		(g_zdt->zoog_free_net_buffer)(znb);
	}
}
