/*
 * \brief  Zoog Monitor -- trusted components of Zoog ABI
 * \author Jon Howell
 * \date   2011-12-22
 */

#include <base/printf.h>
#include <base/env.h>
#include <base/sleep.h>
#include <cap_session/connection.h>
#include <root/component.h>
#include <base/rpc_server.h>
#include <rom_session/connection.h>

#include <nic_session/connection.h>

#include "GBlitProvider.h"
#include "BlitterManager.h"
#include "BlitterViewport.h"
#include "BlitterCanvas.h"
#include "CryptoException.h"
#include "GenodeCanvasAcceptor.h"

#include <zoog_monitor_session/zoog_monitor_session.h>

#include "NetworkThread.h"
#include "standard_malloc_factory.h"
#include "xax_network_utils.h"
#include "hash_table.h"
#include "AllocatedBuffer.h"
#include "DebugFlags.h"
#include "ZBlitter.h"
#include "UIEventQueue.h"
#include "common/SyncFactory_Genode.h"

extern char _binary_zoog_boot_signed_start;
extern char _binary_zoog_boot_signed_end;

using namespace Genode;
namespace ZoogMonitor {
	// TODO separate these by the client we gave them to. (gulp)
	// TODO don't make a separate nic & network thread per client
	class Session_component
		: public Genode::Rpc_object<Session>,
		  public BlitPalIfc
	{
	private:
		class UIEventTransmitter : public EventSynchronizerIfc {
		private:
			Signal_transmitter *xmit;
			bool started;
			uint32_t last_seq_number;
		public:
			UIEventTransmitter(Signal_transmitter *xmit);
			void update_event(AlarmEnum alarm_num, uint32_t sequence_number);
		};

		HashTable _allocated_buffer_table;
		Session::Zoog_genode_net_buffer_id _next_id;
		Genode::Allocator_avl _tx_block_alloc;
		Nic::Connection _nic;

		AllocatedBuffer *_retrieve_buffer(Session::Zoog_genode_net_buffer_id buffer_id);
		void release_buffer(AllocatedBuffer *allocated_buffer);
		XIPifconfig _ifconfig;	// TODO just one for now
		static XIPifconfig default_ifconfig();
		ZoogMonitor::NetworkThread _network_thread;
		Signal_transmitter network_receive_signal_transmitter;
		Signal_transmitter ui_event_signal_transmitter;
		Signal_transmitter core_dump_invoke_signal_transmitter;
		DebugFlags _dbg_flags;

		ZoogMonitor::Session::Dispatch *dispatch_region;
		uint32_t dispatch_region_count;

		MallocFactory *mf;
		SyncFactory_Genode sf;

		UIEventTransmitter uiet;
		UIEventQueue uieq;
		BlitterManager *blitter_manager;
		ZPubKey *pub_key;

		static void dbg_print_buffer_ids(void *user_obj, void *a);

		void get_ifconfig(ZoogMonitor::Session::DispatchGetIfconfig *arg);
		void send_net_buffer(ZoogMonitor::Session::DispatchSendNetBuffer *arg);
		void free_net_buffer(ZoogMonitor::Session::DispatchFreeNetBuffer *arg);
		void update_canvas(ZoogMonitor::Session::DispatchUpdateCanvas *arg);
		void receive_ui_event(ZoogMonitor::Session::DispatchReceiveUIEvent *arg);
		void update_viewport(ZoogMonitor::Session::DispatchUpdateViewport *arg);
		void close_canvas(ZoogMonitor::Session::DispatchCloseCanvas *arg);

	public:
		Session_component(BlitterManager *blitter_manager, Timer::Connection *timer);
		~Session_component();

		Dataspace_capability map_dispatch_region(uint32_t region_count);
		Dataspace_capability get_boot_block();
		Dataspace_capability alloc_net_buffer(uint32_t payload_size);
		ZoogMonitor::Session::Receive_net_buffer_reply receive_net_buffer();
		void event_receive_sighs(Signal_context_capability network_cap, Signal_context_capability ui_cap);
		void core_dump_invoke_sigh(Signal_context_capability cap);
		AcceptCanvasReply accept_canvas(ViewportID viewport_id);
		void dispatch_message(uint32_t index);

	public:
		// BlitPalIfc
		UIEventDeliveryIfc *get_uidelivery() { return &uieq; }
		ZPubKey *get_pub_key() { return pub_key; }
		void debug_remark(const char *msg) { PDBG("Blitter sez '%s'", msg); }
	};

	class Root_component : public Genode::Root_component<Session_component>
	{
		protected:
			MallocFactory *mf;
			SyncFactory_Genode sf;
			GBlitProvider gblit_provider;
			Timer::Connection timer;
			BlitterManager blitter_manager;

			ZoogMonitor::Session_component *_create_session(const char *args)
			{
				PDBG("creating ZoogMonitor session.");
				return new (md_alloc()) Session_component(&blitter_manager, &timer);
			}

		public:

			Root_component(Genode::Rpc_entrypoint *ep,
			               Genode::Allocator *allocator)
			: Genode::Root_component<Session_component>(ep, allocator),
			  mf(standard_malloc_factory_init()),
			  gblit_provider(&timer),
			  blitter_manager(&gblit_provider, mf, &sf)
			{
				PDBG("Creating root component.");
			}
	};
}

ZoogMonitor::Session_component::UIEventTransmitter::UIEventTransmitter(
	Signal_transmitter *xmit)
	: xmit(xmit),
	  started(false)
{}

void ZoogMonitor::Session_component::UIEventTransmitter::update_event(
	AlarmEnum alarm_num, uint32_t sequence_number)
{
	if (!started)
	{
		PDBG("transmit first");
		xmit->submit();
	}
	else
	{
		uint32_t num_bumps = sequence_number - last_seq_number;
		lite_assert(num_bumps==1);	// no reason it needs to be, but I think that's how UIEq works, so I'll assert it because I'm in the mood.
		PDBG("transmit %d more", num_bumps);
		xmit->submit(num_bumps);
	}
	started = true;
	last_seq_number = sequence_number;
}

ZoogMonitor::Session_component::Session_component(BlitterManager *blitter_manager, Timer::Connection *timer)
	: _next_id(1),
	  _tx_block_alloc(env()->heap()),
	  _nic(&_tx_block_alloc),
	  _ifconfig(default_ifconfig()),
	  _network_thread(&_nic, &_ifconfig, &network_receive_signal_transmitter, &core_dump_invoke_signal_transmitter, &_dbg_flags, timer),
	  dispatch_region(0),
	  dispatch_region_count(0),
	  mf(standard_malloc_factory_init()),
	  uiet(&ui_event_signal_transmitter),
	  uieq(mf, &sf, &uiet),
	  blitter_manager(blitter_manager),
	  pub_key(NULL)
{
	hash_table_init(&_allocated_buffer_table,
		standard_malloc_factory_init(),
	AllocatedBuffer::hash,
	AllocatedBuffer::cmp);
	_network_thread.start();
}

ZoogMonitor::Session_component::~Session_component()
{
	blitter_manager->unregister_palifc(this);
}

XIPifconfig ZoogMonitor::Session_component::default_ifconfig()
{
	XIPifconfig result;
	result.local_interface = literal_ipv4xip(10,200,0,3);
	result.gateway = literal_ipv4xip(10,200,0,1);
	result.netmask = literal_ipv4xip(255,255,255,0);
	return result;
}

using namespace ZoogMonitor;

Dataspace_capability ZoogMonitor::Session_component::map_dispatch_region(uint32_t region_count)
{
	lite_assert(region_count < 100);	// something went crazy.
	this->dispatch_region_count = region_count;
	uint32_t region_size = sizeof(dispatch_region[0]) * region_count;
	// TODO bill this region to the client
	Dataspace_capability dataspace_cap =
		env()->ram_session()->alloc(region_size);
	dispatch_region = (ZoogMonitor::Session::Dispatch *)
		env()->rm_session()->attach(dataspace_cap);

	return dataspace_cap;
}

Dataspace_capability ZoogMonitor::Session_component::get_boot_block()
{
	PDBG("ZoogMonitor get_boot_block.");

	SignedBinary *sb = (SignedBinary *) &_binary_zoog_boot_signed_start;
	uint32_t cert_len = Z_NTOHG(sb->cert_len);
	uint32_t binary_len = Z_NTOHG(sb->binary_len);
	uint32_t sb_len = sizeof(*sb) + cert_len + binary_len;
	uint32_t cd_len = &_binary_zoog_boot_signed_end - &_binary_zoog_boot_signed_start;
	lite_assert(sb_len == cd_len);

	uint8_t *cert_start = (uint8_t*) (&sb[1]);

	// record the cert to represent client
	PDBG("cert_len %d binary_len %d bytes %02x %02x %02x %02x",
		cert_len, binary_len,
		cert_start[0],
		cert_start[1],
		cert_start[2],
		cert_start[3]);
	try {
		ZCert *cert = new ZCert(cert_start, cert_len);
		pub_key = cert->getEndorsingKey();
	} catch (CryptoException ex) {
		PDBG("CryptoException: '%s' %s:%d",
			ex.get_str(),
			ex.get_filename(),
			ex.get_linenum());
		lite_assert(false);
	}

	blitter_manager->register_palifc(this);

	// copy the boot block into a dataspace given to client
	void *boot_block_start = ((char*)cert_start) + cert_len;
	Dataspace_capability dataspace_cap =
		env()->ram_session()->alloc(binary_len);
	uint8_t *boot_block_addr =
		(uint8_t*) env()->rm_session()->attach(dataspace_cap);
	memcpy(boot_block_addr, boot_block_start, binary_len);
	env()->rm_session()->detach(boot_block_addr);

	// welcome, client! Have a window.
	lite_assert(pub_key!=NULL);	// whoah, weird -- PAL should have called get_boot_block already
	blitter_manager->new_toplevel_viewport(pub_key);
		
	return dataspace_cap;
}

Dataspace_capability ZoogMonitor::Session_component::alloc_net_buffer(uint32_t payload_size)
{
	AllocatedBuffer *allocated_buffer = new AllocatedBuffer(_next_id++, payload_size);
//	PDBG("_allocated_buffer_table insert %d", allocated_buffer->buffer_id());
	hash_table_insert(&_allocated_buffer_table, allocated_buffer);
	return allocated_buffer->cap();
}

void ZoogMonitor::Session_component::dbg_print_buffer_ids(void *user_obj, void *a)
{
	AllocatedBuffer *abuf = (AllocatedBuffer *) a;
	PDBG("  one has buffer_id 0x%x", abuf->buffer_id());
}

AllocatedBuffer *ZoogMonitor::Session_component::_retrieve_buffer(Session::Zoog_genode_net_buffer_id buffer_id)
{
	AllocatedBuffer key(buffer_id);
	AllocatedBuffer *allocated_buffer =
		(AllocatedBuffer *) hash_table_lookup(&_allocated_buffer_table, &key);
	if (allocated_buffer == NULL)
	{
		PERR("can't find buffer by id 0x%x; table size %d", buffer_id, _allocated_buffer_table.count);
		hash_table_visit_every(&_allocated_buffer_table, dbg_print_buffer_ids, NULL);
	}
	return allocated_buffer;
}

void ZoogMonitor::Session_component::release_buffer(AllocatedBuffer *allocated_buffer) {
//	PDBG("_allocated_buffer_table remove %d", allocated_buffer->buffer_id());
	hash_table_remove(&_allocated_buffer_table, allocated_buffer);
	delete allocated_buffer;
}

// This version of the interface blocks; it's pal's job to use a thread
// to convert the blockage into an alarmy nonblocking interface.
// Nooooo, we can't block, or we wedge the server. (wait, how do zutexes ... oh, they
// run inside the PAL. D'oh.)
ZoogMonitor::Session::Receive_net_buffer_reply ZoogMonitor::Session_component::receive_net_buffer()
{
	Receive_net_buffer_reply reply;

	Packet *packet = _network_thread.receive();
	if (packet==NULL)
	{
		reply.valid = false;
	}
	else
	{
		AllocatedBuffer *allocated_buffer = new AllocatedBuffer(_next_id++, packet->size);
		hash_table_insert(&_allocated_buffer_table, allocated_buffer);
		memcpy(allocated_buffer->payload(), packet->payload, packet->size);
#if 0
		{
			uint8_t *payload = (uint8_t*) allocated_buffer->payload();
			uint32_t i;
			for (i=0; i<packet->size; i++)
			{
				PDBG("packet[%d] = %02x", i, payload[i]);
			}
		}
#endif
		_network_thread.release(packet);
		reply.valid = true;
		reply.cap = allocated_buffer->cap();
	}
	if (_dbg_flags.packets)
	{
		PDBG("returning reply (valid=%d)", reply.valid);
	}
	return reply;
}

void ZoogMonitor::Session_component::event_receive_sighs(Signal_context_capability network_cap, Signal_context_capability ui_cap)
{
	network_receive_signal_transmitter = Signal_transmitter(network_cap);
	ui_event_signal_transmitter = Signal_transmitter(ui_cap);
}

void ZoogMonitor::Session_component::core_dump_invoke_sigh(Signal_context_capability cap)
{
	core_dump_invoke_signal_transmitter = Signal_transmitter(cap);
}

ZoogMonitor::Session::AcceptCanvasReply ZoogMonitor::Session_component::accept_canvas(ViewportID viewport_id)
{
	AcceptCanvasReply acr;
	GenodeCanvasAcceptor acceptor(&acr);
	BlitterViewport *viewport = blitter_manager->get_viewport(viewport_id);
	lite_assert(viewport!=NULL);
	PixelFormat pixel_format = zoog_pixel_format_truecolor24;
	viewport->accept_canvas(this, &pixel_format, 1, &acceptor);
	return acr;
}

void ZoogMonitor::Session_component::get_ifconfig(
	ZoogMonitor::Session::DispatchGetIfconfig *arg)
{
	arg->out.num_ifconfigs = 1;
	arg->out.ifconfigs[0] = _ifconfig;
}

void ZoogMonitor::Session_component::send_net_buffer(
	ZoogMonitor::Session::DispatchSendNetBuffer *arg)
{
//	PDBG("send_net_buffer(%d, %d)", buffer_id, release);
	AllocatedBuffer *allocated_buffer = _retrieve_buffer(arg->in.buffer_id);
	if (allocated_buffer == NULL) { return; }
	// ugh, gotta copy, huh? Could avoid this with some "clever" reservation
	// of space in front of / overlapping the ZNB. meh for now.
	_network_thread.send_packet(allocated_buffer);
	if (arg->in.release) {
		release_buffer(allocated_buffer);
	}
}

void ZoogMonitor::Session_component::free_net_buffer(
	ZoogMonitor::Session::DispatchFreeNetBuffer *arg)
{
//	PDBG("free_net_buffer(%d)", buffer_id);
	AllocatedBuffer *allocated_buffer = _retrieve_buffer(arg->in.buffer_id);
	if (allocated_buffer == NULL) { return; }
	release_buffer(allocated_buffer);
}

void ZoogMonitor::Session_component::update_canvas(
	ZoogMonitor::Session::DispatchUpdateCanvas *arg)
{
	BlitterCanvas *canvas = blitter_manager->get_canvas(arg->in.canvas_id);
	lite_assert(canvas!=NULL);
	WindowRect wr(arg->in.x, arg->in.y, arg->in.width, arg->in.height);
	canvas->update_canvas(&wr);
}

void ZoogMonitor::Session_component::receive_ui_event(
	ZoogMonitor::Session::DispatchReceiveUIEvent *arg)
{
	ZoogUIEvent *evt = uieq.remove();
	if (evt!=NULL)
	{
		arg->out.evt = *evt;
	}
	else
	{
		arg->out.evt.type = zuie_no_event;
	}
}

void ZoogMonitor::Session_component::update_viewport(
	ZoogMonitor::Session::DispatchUpdateViewport *arg)
{ PDBG("unimpl"); lite_assert(false); }

void ZoogMonitor::Session_component::close_canvas(
	ZoogMonitor::Session::DispatchCloseCanvas *arg)
{ PDBG("unimpl"); lite_assert(false); }


void ZoogMonitor::Session_component::dispatch_message(uint32_t index)
{
	lite_assert(index < dispatch_region_count);
	ZoogMonitor::Session::Dispatch *d = &dispatch_region[index];
//	PDBG("opcode %d", d->opcode);
	switch (d->opcode)
	{
	case do_get_ifconfig:
		get_ifconfig(&d->un.get_ifconfig);
		break;
	case do_send_net_buffer:
		send_net_buffer(&d->un.send_net_buffer);
		break;
	case do_free_net_buffer:
		free_net_buffer(&d->un.free_net_buffer);
		break;
	case do_update_canvas:
		update_canvas(&d->un.update_canvas);
		break;
	case do_receive_ui_event:
		receive_ui_event(&d->un.receive_ui_event);
		break;
	case do_update_viewport:
		update_viewport(&d->un.update_viewport);
		break;
	case do_close_canvas:
		close_canvas(&d->un.close_canvas);
		break;
	}
//	PDBG("done opcode %d", d->opcode);
}

int main(void)
{
	/*
	 * Get a session for the parent's capability service, so that we
	 * are able to create capabilities.
	 */
	Cap_connection cap;

#if 0
	Nic::Session::Tx::Source *source = nic.tx_channel()->source();
	Packet_descriptor packet = source->alloc_packet(1000);
	char *content = source->packet_content(packet);
	memset(content, 6, packet.size());
	source->submit_packet(packet);
#endif


	/*
	 * A sliced heap is used for allocating session objects - thereby we
	 * can release objects separately.
	 */
	static Sliced_heap sliced_heap(env()->ram_session(),
	                               env()->rm_session());

	/*
	 * Create objects for use by the framework.
	 *
	 * An 'Rpc_entrypoint' is created to announce our service's root
	 * capability to our parent, manage incoming session creation
	 * requests, and dispatch the session interface. The incoming RPC
	 * requests are dispatched via a dedicated thread. The 'STACK_SIZE'
	 * argument defines the size of the thread's stack. The additional
	 * string argument is the name of the entry point, used for
	 * debugging purposes only.
	 */
	enum { STACK_SIZE = 4096 };
	static Rpc_entrypoint ep(&cap, STACK_SIZE, "zoog_monitor_ep");

#if 0
	uint8_t *paltest_bytes = (uint8_t*) &_binary_zoog_boot_raw_start;
	printf("paltest at %08x\n", (uint32_t) paltest_bytes);
	{
		int i;
		for (i=0; i<10; i++)
		{
			printf("    byte[%2d] = %02x\n", i, paltest_bytes[i]);
		}
	}
#endif

	static ZoogMonitor::Root_component zoog_monitor_root(&ep, &sliced_heap);
	env()->parent()->announce(ep.manage(&zoog_monitor_root));

	ZBlitter zblitter;

	/* We are done with this and only act upon client requests now. */
	sleep_forever();

	return 0;
}
