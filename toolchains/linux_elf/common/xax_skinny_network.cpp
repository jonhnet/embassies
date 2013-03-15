/*
#include <sys/types.h>
#include <malloc.h>
#include <stdio.h>
#include <time.h>
*/

#include "LiteLib.h"
#include "xax_skinny_network.h"
#include "cheesy_thread.h"
#include "zutex_sync.h"
#include "xax_endian.h"
#include "pal_abi/pal_extensions.h"
#include "debug_util.h"
#include "simple_network.h"
#include "xax_network_utils.h"
#include "ZeroCopyBuf_Xnb.h"
#include "xax_util.h"
#include "ZeroCopyView.h"
#include "ZLCEmitXdt.h"
#include "zemit.h"

#define REJECT(s)	ZLC_COMPLAIN(ze, "rejected packet; %s\n",, (s));

#if 0 // dead code
void udp_send_packet(XaxSkinnyNetwork *xsn,
	UDPEndpoint source_ep, UDPEndpoint dest_ep, char *data, int data_length)
{
	void *payload;
	ZoogNetBuffer *xb = create_udp_packet_xnb(xsn->xdt,
		source_ep, dest_ep, data_length, &payload);
	lite_memcpy(payload, data, data_length);
	(xsn->xdt->zoog_send_net_buffer)(xb, true);
}
#endif

#define UDP_PORT_UNDEF 0

void XaxSkinnyNetwork::_network_dispatcher_thread_s(void *v_xsn)
{
	XaxSkinnyNetwork *xsn = (XaxSkinnyNetwork *) v_xsn;
	xsn->_network_dispatcher_thread();
}

ZoogNetBuffer *XaxSkinnyNetwork::_blocking_receive()
{
	ZoogNetBuffer *znb;
	while (1)
	{
		uint32_t match_val = alarms->receive_net_buffer;
		znb = (zdt->zoog_receive_net_buffer)();
		if (znb!=NULL) { break; }
		ZutexWaitSpec spec = { &alarms->receive_net_buffer, match_val };
		(zdt->zoog_zutex_wait)(&spec, 1);
	}
	return znb;
}

void XaxSkinnyNetwork::_network_dispatcher_thread()
{
	ZoogNetBuffer *znb = NULL;

	while (1)
	{
		znb = _blocking_receive();

		IPInfo info;
		decode_ip_packet(&info, ZNB_DATA(znb), znb->capacity);
		if (!info.packet_valid)
		{
			REJECT("Packet invalid; dropped");
			(zdt->zoog_free_net_buffer)(znb);
			continue;
		}
		
		if (test_random_drops())
		{
			REJECT("test_random_drops");
			(zdt->zoog_free_net_buffer)(znb);
			continue;
		}

#define QUEUE_SIZE (20)
		if (_receive_accounting.get_queue_depth() > QUEUE_SIZE)
		{
			REJECT("queue_full");	// Probably a hint that someone's forgetting to delete the zcbs he's getting from received packets.
			(zdt->zoog_free_net_buffer)(znb);
			continue;
		}

		ZeroCopyBuf *zcb =
			new ZeroCopyBuf_XSNRA(get_xdt(), znb, &_receive_accounting);
//			new ZeroCopyBuf_Xnb(get_xdt(), znb, 0, znb->capacity);

		if (info.more_fragments || info.fragment_offset>0)
		{
			fragment_reassembly.handle_fragment(zcb, &info);
		}
		else
		{
			dispatch_packet(zcb, &info);
		}
	}
}

bool XaxSkinnyNetwork::test_random_drops()
{
	_random_drop_state = _random_drop_state + 1;
	return _enable_random_drops && ((_random_drop_state % 617) == 0);
}

void XaxSkinnyNetwork::dispatch_packet(ZeroCopyBuf *zcb, IPInfo *info)
{
	_mutex->lock();

	PFRegistration pr;
	pr.ipver = info->dest.version;
	pr.ip_protocol = info->layer_4_protocol;
	pr.udp_port = 0;
	if (info->layer_4_protocol == PROTOCOL_UDP)
	{
		UDPPacket *udp = (UDPPacket*) (zcb->data()+info->payload_offset);
		pr.udp_port = Z_NTOHG(udp->dest_port);
	}

	PFHandler *ph;
	ph = pftable_lookup(&pftable, &pr);
	if (ph==NULL)
	{
		PFRegistration prdefault;
		prdefault.ipver = IPVER_ANY;
		prdefault.ip_protocol = PROTOCOL_DEFAULT;
		prdefault.udp_port = UDP_PORT_UNDEF;

		ph = pftable_lookup(&pftable, &prdefault);
	}
	if (ph==NULL)
	{
		REJECT("Packet dropped due to lack of interest.\n");
		delete zcb;
		goto exit;
	}
	if (ph==clock_sentinel)
	{
		lite_assert(false);
		delete zcb;
		goto exit;
	}

	ZLC_CHATTY(ze, "Delivering %d-byte packet to udp port %d\n",,
		info->packet_length, pr.udp_port);

	//ZLC_TERSE(ze, "Packet to mq %08x\n",, &ph->mq);

	// found a queue to deliver the message on.
	MQEntry *new_msg;
	new_msg = (MQEntry*) mf_malloc(mf, sizeof(MQEntry));
	new_msg->zcb = zcb;
	new_msg->next = NULL;
	new_msg->info = *info;
	mq_append(&ph->mq, new_msg);

exit:
	_mutex->unlock();
	return;
}

ZClock *XaxSkinnyNetwork::get_clock()
{
	return &_zclock;
}

SyncFactory *XaxSkinnyNetwork::get_timer_sf()
{
	return &_timer_sf;
}

void XaxSkinnyNetwork::free_packet_filter(PFHandler *ph)
{
	pftable_remove(&pftable, ph);
	pfhandler_free(ph);
}

void network_free_packet_filter(PFHandler *ph)
{
	XaxSkinnyNetwork *xsn = (XaxSkinnyNetwork *) ph->xsn;
	xsn->free_packet_filter(ph);
}

uint16_t XaxSkinnyNetwork::allocate_port_number()
{
	_mutex->lock();
	uint16_t port = next_port++;
	_mutex->unlock();
	return port;
}

uint16_t xsn_allocate_port_number(XaxSkinnyNetwork *xsn)
{
	return xsn->allocate_port_number();
}

PFHandler *XaxSkinnyNetwork::network_register_handler(uint8_t ipver, uint8_t ip_protocol, uint16_t udp_port)
{
	_mutex->lock();
	PFHandler *ph = pfhandler_init(&this->placebo.placebo, ipver, ip_protocol, udp_port);
	pftable_insert(&pftable, ph);
	_mutex->unlock();
	return ph;
}

void XaxSkinnyNetwork::unregister(PFHandler *ph)
{
	_mutex->lock();
	pftable_remove(&pftable, ph);
	_mutex->unlock();
	// todo free port number.
}

PFHandler *xsn_network_register_handler(XaxSkinnyNetwork *xsn, uint8_t ipver, uint8_t ip_protocol, uint16_t udp_port)
{
	return xsn->network_register_handler(ipver, ip_protocol, udp_port);
}

PFHandler *xsn_network_add_default_handler(XaxSkinnyNetwork *xsn)
{
	return xsn_network_register_handler(xsn, IPVER_ANY, PROTOCOL_DEFAULT, UDP_PORT_UNDEF);
}

XIPifconfig *XaxSkinnyNetwork::get_ifconfig(XIPVer ipver)
{
	int i;
	for (i=0; i<num_ifconfigs; i++)
	{
		if (ifconfigs[i].local_interface.version == ipver)
		{
			return &ifconfigs[i];
		}
	}
	return NULL;
}

ZoogDispatchTable_v1 *XaxSkinnyNetwork::get_xdt()
{
	return zdt;
}

MallocFactory *XaxSkinnyNetwork::get_malloc_factory()
{
	return mf;
}

XaxSkinnyNetwork::XaxSkinnyNetwork(ZoogDispatchTable_v1 *zdt, MallocFactory *mf)
	: ze(new ZLCEmitXdt(zdt, terse)),
	  _simple_sf(zdt),
	  _mutex(_simple_sf.new_mutex(false)),
	  _zclock(zdt, &_simple_sf),
	  _timer_sf(zdt, &_zclock),
	  fragment_reassembly(this, mf, &_simple_sf, ze),
	  _enable_random_drops(false),
	  _random_drop_state(0),
	  _receive_accounting(mf, &_simple_sf)
{
	this->zdt = zdt;
	this->mf = mf;

	// placebo must be in place before we begin using PacketFilter.
	placebo.placebo.methods = &placebo_methods;
	placebo.x_this = this;

	xsxdt_init(&xstream, zdt, "stderr");

	pftable_init(&pftable, mf);
	next_port = 1201;

	num_ifconfigs = sizeof(ifconfigs)/sizeof(ifconfigs[0]);
	(zdt->zoog_get_ifconfig)(ifconfigs, &num_ifconfigs);
	lite_assert(num_ifconfigs>0);	// sad day; no network

	alarms = (zdt->zoog_get_alarms)();

	// tick delivery routine special-cases this handler, never adding to its
	// queue. We used to use a bunch of uuuugly C polymorphism, but if
	// we want to do that again, let's just tough up and convert this
	// class to C++. For now, we special-case.
	clock_sentinel =
		network_register_handler(IPVER_ANY, PROTOCOL_UDP, UDP_PORT_FAKE_TIMER);

	// start the listener thread processing incoming packets
	cheesy_thread_create(zdt, _network_dispatcher_thread_s, this, 32<<10);
}

void XaxSkinnyNetwork::enable_random_drops(bool enable)
{
	_enable_random_drops = enable;
}

XaxSkinnyNetworkPlacebo_Methods XaxSkinnyNetwork::placebo_methods =
	{ XaxSkinnyNetwork::get_xdt, XaxSkinnyNetwork::get_malloc_factory };

bool xax_skinny_socket_init(XaxSkinnySocket *skinny, XaxSkinnyNetwork *xsn, uint16_t port_host_order, XIPVer ipver)
{
	skinny->xsn = xsn;
	if (port_host_order!=0)
	{
		// if this port is already claimed, we'll explode in
		// pftable_insert.
		skinny->local_port_host_order = port_host_order;
	}
	else
	{
		skinny->local_port_host_order = xsn_allocate_port_number(xsn);
	}

	XIPifconfig *ifconfig = xsn->get_ifconfig(ipver);
	if (ifconfig==NULL)
	{
		skinny->valid = false;
	} else {
		skinny->valid = true;
		skinny->local_ep.ipaddr = ifconfig->local_interface;
		skinny->local_ep.port = z_htons(skinny->local_port_host_order);
	}

	skinny->ph = xsn_network_register_handler(
		xsn, ipver, PROTOCOL_UDP, skinny->local_port_host_order);

	return skinny->valid;
}

void xax_skinny_socket_free(XaxSkinnySocket *skinny)
{
	skinny->xsn->unregister(skinny->ph);
	pfhandler_free(skinny->ph);
}

ZoogNetBuffer *xax_skinny_socket_prepare_packet(XaxSkinnySocket *skinny, UDPEndpoint dest_ep, int payload_size, void **out_payload)
{
	return create_udp_packet_xnb(skinny->xsn->get_xdt(), skinny->local_ep, dest_ep, payload_size, out_payload);
}

void xax_skinny_socket_send(XaxSkinnySocket *skinny, ZoogNetBuffer *znb, bool release)
{
	(skinny->xsn->get_xdt()->zoog_send_net_buffer)(znb, release);
}

// the deprecated memcpy ifc
int xax_skinny_socket_recv_buf(XaxSkinnySocket *skinny, uint8_t *buf, uint32_t data_len, UDPEndpoint *sender_ep)
{
	ZeroCopyBuf *zcb = xax_skinny_socket_recv_packet(skinny, sender_ep);
	if (zcb==NULL)
	{
		return 0;
	}
	uint32_t len = zcb->len();
	lite_assert(len <= data_len);
	lite_memcpy(buf, zcb->data(), len);
	delete zcb;
	return len;
}

ZeroCopyBuf *xax_skinny_socket_recv_packet(XaxSkinnySocket *skinny, UDPEndpoint *sender_ep)
{
	ZAS *zasses[2];

	// Set up MQ zas
	zasses[0] = &skinny->ph->mq.zas;

	// Set up timer ZAS
	ZClock *zclock = skinny->xsn->get_clock();
	uint64_t alarm_time = zclock->get_time_zoog();
	alarm_time += 1000000000LL;
	ZTimer ztimer(zclock, alarm_time);

	zasses[1] = ztimer.get_zas();

	ZeroCopyBuf *zcb;
	int idx;
	while (1)
	{
		idx = zas_wait_any(skinny->xsn->get_xdt(), zasses, 2);

		if (idx==1)
		{
			// timer tick.
//			write(2, "tock.\n", lite_strlen("tock.\n"));
			zcb = NULL;
			break;
		}
		else
		{
			// shouldn't block, since mq's zas check just returned true,
			// and there are no other consuming threads.
			MQEntry *msg = mq_pop(&skinny->ph->mq, 0);
			if (msg==NULL)
			{
#if 1
				const char *m = "WARN: mq ready, but nothing there!\n";
				int _xaxunder___write(int, const char *, int);
				debug_logfile_append_f *dla = (debug_logfile_append_f *)
					(skinny->xsn->get_xdt()->zoog_lookup_extension)("debug_logfile_append");
				(dla)("stack_bases", m);
				continue;
#endif
			}
			xax_assert(msg->zcb != NULL);	// deprecated "interface" -- passing empty mq
			xax_assert(msg->zcb->len() >= msg->info.packet_length);
				// this check duplicates what is now done in decode_ip_packet()

			sender_ep->ipaddr = msg->info.source;
			UDPPacket *udp = (UDPPacket*) (msg->zcb->data()+msg->info.payload_offset);
			sender_ep->port = udp->source_port;	// preserve network order

			zcb = new ZeroCopyView(msg->zcb, true, msg->info.payload_offset + sizeof(UDPPacket), msg->info.payload_length - sizeof(UDPPacket));
				// view zcb now responsible for freeing backing zcb.
			msg->zcb = NULL;	// we've got it, thanks.
				// Ugh. Yes. I'm communicating the ownership change by
				// reaching into the object and nulling its field. Yuck.

			cheesy_free(msg);
			break;
		}
	}

	// NB ztimer goes out of scope here.

	return zcb;
}

ZoogDispatchTable_v1 *XaxSkinnyNetwork::get_xdt(XaxSkinnyNetworkPlacebo *xsnp)
{
	struct s_placebo *sp = (struct s_placebo *) xsnp;
	return sp->x_this->get_xdt();
}

MallocFactory *XaxSkinnyNetwork::get_malloc_factory(XaxSkinnyNetworkPlacebo *xsnp)
{
	struct s_placebo *sp = (struct s_placebo *) xsnp;
	return sp->x_this->get_malloc_factory();
}

