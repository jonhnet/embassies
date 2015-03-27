#include "throughputtest.h"
#include "simple_network.h"
#include "xax_network_utils.h"
#include "cheesy_snprintf.h"
#include "zoog_malloc_factory.h"
#include "xax_skinny_network.h"
#include "SyncFactory_Zutex.h"
#include "throughput_test_header.h"

#define local_assert(x)	{ if (!(x)) { msg(#x); } lite_assert(x); }

ThroughputTest::ThroughputTest(
		ZoogDispatchTable_v1 *zdt,
		XaxSkinnyNetwork *xsn,
		XIPAddr *server)
	: zdt(zdt),
	  xsn(xsn),
	  debuglog((debug_logfile_append_f *)
	  	(zdt->zoog_lookup_extension)("debug_logfile_append")),
	server(server)
{
}

void ThroughputTest::msg(const char *m)
{
	(debuglog)("stderr", m);
}

void ThroughputTest::setup()
{
	bool rc = xax_skinny_socket_init(&skinny_socket, xsn, 0, ipv4);
	local_assert(rc);

	UDPEndpoint dst;
	dst.ipaddr = *server;
	dst.port = z_htons(7660);
	char *out_payload;
	
	request_znb = xax_skinny_socket_prepare_packet(
		&skinny_socket, dst, 1, (void**) &out_payload);

	out_payload[0] = 'm';
}

void ThroughputTest::ping_throughput_server()
{
	// send without release
	xax_skinny_socket_send(&skinny_socket, request_znb, false);
}

void ThroughputTest::run()
{
	setup();

	received_total = 0;
	uint64_t start_time;
	(zdt->zoog_get_time)(&start_time);
	uint32_t display_period = 1<<20;	// bytes
	uint32_t next_display = display_period;
	uint64_t cur_time;
	ZeroCopyBuf *zcb;
	uint32_t lost = 0;

	Signal signal;
	signal.total = 1;	// dummy value if first thing that happens is timeout.

	while (true)
	{
		ping_throughput_server();

		uint32_t count_this_round = 0;
		// wait for at least one reply packet
		while (true)
		{
			UDPEndpoint sender_ep;
			zcb = xax_skinny_socket_recv_packet(&skinny_socket, &sender_ep);
			if (zcb==NULL)
			{
				msg("timeout.\n");
				break;
			}

			local_assert(sender_ep.port == z_htons(7660));
			local_assert(zcb->len() >= sizeof(Signal));
			signal = ((Signal*) zcb->data())[0];

			// be sure data wasn't reordered by fragment reassembly.
			uint8_t *data = zcb->data();
			uint32_t i,j;
			for (i=sizeof(Signal), j=0; i<zcb->len(); i++, j=(j+1)%217)
			{
				local_assert(data[i]==j);
			}

			received_total += zcb->len();
			delete zcb;
			
			count_this_round+=1;
			if (signal.number==signal.total-1)
			{
				break;
			}
		}
		local_assert(count_this_round <= signal.total);
		if (signal.total > count_this_round)
		{
			lost += (signal.total - count_this_round);
		}


		if (received_total > next_display)
		{
			(zdt->zoog_get_time)(&cur_time);
			double elapsed_secs = (cur_time-start_time)/1000000000.0;
			double bytes_per_sec = ((double)received_total) / elapsed_secs;
			double megabits_per_sec = bytes_per_sec*8/(1024*1024);

			cheesy_snprintf(buf, sizeof(buf),
				"At %dMB, %.2fMbps (%d lost)\n",
				received_total>>20,
				megabits_per_sec,
				lost);
			msg(buf);

			next_display += display_period;
		}
	}
}
