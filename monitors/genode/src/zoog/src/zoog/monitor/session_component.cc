#include "boot_block.h"
#include "session_component.h"
#include "root_component.h"
#include "CryptoException.h"
#include "BlitterViewport.h"
#include "BlitterCanvas.h"
#include "GenodeCanvasAcceptor.h"

ZoogMonitor::Session_component::Session_component(
	Root_component* root_component,
	BlitterManager* blitter_manager,
	Timer::Connection *timer,
	Zoog_genode_process_id zoog_process_id,
	XIPifconfig *ifconfig,
	NetworkThread* network_thread,
	ChannelWriter* channel_writer)
	: channel_writer(channel_writer),
	  _next_id(1),
	  _boot_block(NULL),
	  zoog_process_id(zoog_process_id),
	  _ifconfig(*ifconfig),
	  dispatch_region(0),
	  dispatch_region_count(0),
	  mf(standard_malloc_factory_init()),
	  uiet(&ui_event_signal_transmitter),
	  uieq(mf, &sf, &uiet),
	  root_component(root_component),
	  blitter_manager(blitter_manager),
	  pub_key(NULL),
	  networkTerminus(
	  	zoog_process_id,
		&_ifconfig,
		&network_receive_signal_transmitter,
		&core_dump_invoke_signal_transmitter,
		network_thread)
{
	PDBG("ctor body");
	hash_table_init(&_allocated_buffer_table,
		standard_malloc_factory_init(),
	AllocatedBuffer::hash,
	AllocatedBuffer::cmp);
	PDBG("ctor body end");
}

ZoogMonitor::Session_component::~Session_component()
{
	blitter_manager->unregister_palifc(this);
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

void ZoogMonitor::Session_component::launch(BootBlock* boot_block)
{
	this->_boot_block = boot_block;
	start_gate_signal_transmitter.submit();
}


Dataspace_capability ZoogMonitor::Session_component::get_boot_block()
{
	PDBG("ZoogMonitor get_boot_block.");

	// record the cert to represent client
//	PDBG("cert_len %d binary_len %d bytes %02x %02x %02x %02x",
//		cert_len, binary_len,
//		cert_start[0],
//		cert_start[1],
//		cert_start[2],
//		cert_start[3]);
	if (_boot_block==NULL)
	{
		PDBG("Crap! _boot_block is NULL. Exploding.\n");
		lite_assert(false);
	}

	try {
		pub_key = _boot_block->copy_cert()->getEndorsingKey();
		// leak
	} catch (CryptoException ex) {
		PDBG("CryptoException: '%s' %s:%d",
			ex.get_str(),
			ex.get_filename(),
			ex.get_linenum());
		lite_assert(false);
	}

	blitter_manager->register_palifc(this);

	// copy the boot block into a dataspace given to client
	Dataspace_capability dataspace_cap =
		env()->ram_session()->alloc(_boot_block->binary_len());
	uint8_t *boot_block_addr =
		(uint8_t*) env()->rm_session()->attach(dataspace_cap);
	memcpy(boot_block_addr, _boot_block->binary_start(), _boot_block->binary_len());
	env()->rm_session()->detach(boot_block_addr);
	delete _boot_block;	// don't need local copy of queued bits anymore.

	// welcome, client! Have a window.
	lite_assert(pub_key!=NULL);	// whoah, weird -- PAL should have called get_boot_block already
//	blitter_manager->new_toplevel_viewport(pub_key);
// TODO move this to call path via debug extension
		
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

	Packet *packet = networkTerminus.receive();
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
		networkTerminus.release(packet);
		reply.valid = true;
		reply.cap = allocated_buffer->cap();
	}
	if (root_component->dbg_flags()->packets)
	{
		PDBG("returning reply (valid=%d)", reply.valid);
	}
	return reply;
}

void ZoogMonitor::Session_component::event_receive_sighs(Signal_context_capability network_cap, Signal_context_capability ui_cap)
{
	PDBG("event_receive_sighs\n");
	network_receive_signal_transmitter = Signal_transmitter(network_cap);
	ui_event_signal_transmitter = Signal_transmitter(ui_cap);
}

void ZoogMonitor::Session_component::core_dump_invoke_sigh(Signal_context_capability cap)
{
	core_dump_invoke_signal_transmitter = Signal_transmitter(cap);
}

void ZoogMonitor::Session_component::start_gate_sigh(Signal_context_capability cap)
{
	start_gate_signal_transmitter = Signal_transmitter(cap);
	root_component->queue_session(this);
}

ZoogMonitor::Session::MapCanvasReply ZoogMonitor::Session_component::map_canvas(ViewportID viewport_id)
{
	MapCanvasReply mcr;
	GenodeCanvasAcceptor acceptor(&mcr);
	BlitterViewport *viewport = blitter_manager->get_tenant_viewport(viewport_id, zoog_process_id);
	lite_assert(viewport!=NULL);
	PixelFormat pixel_format = zoog_pixel_format_truecolor24;
	viewport->map_canvas(this, &pixel_format, 1, &acceptor, "TODO label");
	return mcr;
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
	networkTerminus.send_packet(allocated_buffer);
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
	canvas->update_canvas(&arg->in.rect);
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

void ZoogMonitor::Session_component::unmap_canvas(
	ZoogMonitor::Session::DispatchUnmapCanvas *arg)
{ PDBG("unimpl"); lite_assert(false); }

void ZoogMonitor::Session_component::new_toplevel_viewport(
	ZoogMonitor::Session::DispatchNewToplevelViewport *arg)
{
	arg->out.viewport_id = blitter_manager->new_toplevel_viewport(zoog_process_id);
}

uint8_t* ZoogMonitor::Session_component::_receive_data_via_net_buffer(
	uint32_t claimed_size,
	Zoog_genode_net_buffer_id buffer_id,
	uint32_t size_sanity_check,
	ReleaseBuffer release_buffer_req)
	// ensures buffer is freed, regardless of success/failure.
{
	uint8_t* result = NULL;
	AllocatedBuffer *allocated_buffer = NULL;
	ZoogNetBuffer* znb = NULL;
	bool rc = false;

	allocated_buffer = _retrieve_buffer(buffer_id);
	if (allocated_buffer == NULL)
	{
		PDBG("_receive_data_via_net_buffer: yoink! NULL buffer.\n");
		goto fail;
	}

	if (claimed_size > size_sanity_check)
	{
		PDBG("_receive_data_via_net_buffer: buffer size implausible 0x%x > 0x%x\n",
			claimed_size, size_sanity_check);
		goto fail;
	}

	znb = allocated_buffer->znb();
	if (claimed_size > znb->capacity)
	{
		PDBG("_receive_data_via_net_buffer: claimed_size 0x%x, but znb capacity only 0x%x\n",
			claimed_size, znb->capacity);
		goto fail;
	}

	rc = Genode::env()->heap()->alloc(claimed_size, &result);
		// TODO: be Genode-y, and bill this allocation to the calling session.
	if (!rc)
	{
		goto fail;
	}

	memcpy(result, ZNB_DATA(znb), claimed_size);

fail:
	if (release_buffer_req==RELEASE && allocated_buffer!=NULL)
	{
		release_buffer(allocated_buffer);
	}
	return result;
}

void ZoogMonitor::Session_component::verify_label(
	ZoogMonitor::Session::DispatchVerifyLabel *arg)
{
	ZCertChain* chain = NULL;
	bool verified = false;
	bool result = false;
	uint8_t* chain_copy = NULL;

	chain_copy = _receive_data_via_net_buffer(
		arg->in.buffer_size,
		arg->in.buffer_id,
		1<<16,
		RELEASE);
	if (chain_copy==NULL)
	{
		PDBG("verify: _receive_data_via_net_buffer failed\n");
		goto fail;
	}

	try {
		chain = new ZCertChain(chain_copy, arg->in.buffer_size);
	} catch (CryptoException ex) {
		PDBG("verify: chain no parsey: size %d first %x\n", arg->in.buffer_size, chain_copy[0]);
		goto fail;
	}
	verified = get_pub_key()->verifyLabel(
		chain, root_component->get_zoog_ca_root_key());
	if (!verified)
	{
		PDBG("chain size %d byte %x\n", arg->in.buffer_size, chain_copy[0]);
		PDBG("ca size %d\n", root_component->get_zoog_ca_root_key()->size());
		PDBG("verify: failed!\n");
		goto fail;
	}

	PDBG("verify: succeeded at server!\n");
	result = true;
fail:
	if (chain!=NULL)
	{
		delete chain;
	}
	if (chain_copy!=NULL)
	{
		Genode::env()->heap()->free(chain_copy, arg->in.buffer_size);
	}
	arg->out.result = result;
}

void ZoogMonitor::Session_component::sublet_viewport(
	ZoogMonitor::Session::DispatchSubletViewport *arg)
{
	BlitterViewport *parent_viewport = blitter_manager->get_tenant_viewport(arg->in.tenant_viewport, zoog_process_id);
	BlitterViewport *child_viewport = parent_viewport->sublet(&arg->in.rectangle, zoog_process_id);

	arg->out.landlord_viewport = child_viewport->get_landlord_id();
	arg->out.deed = child_viewport->get_deed();
}

void ZoogMonitor::Session_component::accept_viewport(
	ZoogMonitor::Session::DispatchAcceptViewport *arg)
{
	BlitterViewport *viewport = blitter_manager->accept_viewport(arg->in.deed, zoog_process_id);	// this lookup is global, as the deed is a cryptographic capability
	lite_assert(viewport!=NULL);	// safety: deed was bogus

	arg->out.tenant_viewport = viewport->get_tenant_id();
	arg->out.deed_key = viewport->get_deed_key();
}

void ZoogMonitor::Session_component::repossess_viewport(
	ZoogMonitor::Session::DispatchRepossessViewport *arg)
{
	BlitterViewport *landlord_viewport = blitter_manager->get_landlord_viewport(arg->in.landlord_viewport, zoog_process_id);
	delete landlord_viewport;
}

void ZoogMonitor::Session_component::get_deed_key(
	ZoogMonitor::Session::DispatchGetDeedKey *arg)
{
	BlitterViewport *landlord_viewport = blitter_manager->get_landlord_viewport(arg->in.landlord_viewport, zoog_process_id);

	arg->out.deed_key = landlord_viewport->get_deed_key();
}

void ZoogMonitor::Session_component::transfer_viewport(
	ZoogMonitor::Session::DispatchTransferViewport *arg)
{
	BlitterViewport *tenant_viewport = blitter_manager->get_tenant_viewport(arg->in.tenant_viewport, zoog_process_id);
	lite_assert(tenant_viewport->get_tenant_id()==arg->in.tenant_viewport);	// safety: tenant_viewport handle was a landlord handle; what you tryin' to pull?

	arg->out.deed = tenant_viewport->transfer();
}

void ZoogMonitor::Session_component::launch_application(
	ZoogMonitor::Session::DispatchLaunchApplication *arg)
{
	uint8_t* signed_binary_copy = _receive_data_via_net_buffer(
		arg->in.buffer_size,
		arg->in.buffer_id,
		5<<20,
		RELEASE);

	SignedBinary* signed_binary = (SignedBinary*) signed_binary_copy;
	if (signed_binary_copy!=NULL)
	{
		BootBlock* boot_block = new BootBlock(signed_binary, arg->in.buffer_size);
		root_component->queue_boot_block(boot_block);
		Genode::env()->heap()->free(signed_binary_copy, arg->in.buffer_size);
	}
}

void ZoogMonitor::Session_component::allocate_memory(
	ZoogMonitor::Session::Dispatch *d,
	ZoogMonitor::Session::DispatchAllocateMemory *arg)
{
	// Dear Norman Feske: if you ever find yourself reading this
	// code, you'll see that I've basically taken everything your
	// system design holds dear, and run it over with a truck.
	// An ugly, rusty truck. Please accept my apologies. I was in
	// a hurry, and correct allocation management was too low on
	// my priority list. Still, I'm ashamed of myself. Consider
	// this an IOU for a beer.
	d->out_dataspace_capability = env()->ram_session()->alloc(arg->in.length);
}

void ZoogMonitor::Session_component::debug_write_channel_open(
	ZoogMonitor::Session::DispatchDebugWriteChannelOpen *arg)
{
	uint8_t* channel_name = _receive_data_via_net_buffer(
		arg->in.channel_name_size,
		arg->in.buffer_id,
		0x100,
		KEEP);
	channel_writer->open((char*) channel_name, arg->in.data_size);
	Genode::env()->heap()->free(channel_name, arg->in.channel_name_size);
}

void ZoogMonitor::Session_component::debug_write_channel_write(
	ZoogMonitor::Session::DispatchDebugWriteChannelWrite *arg)
{
	uint8_t* bytes = _receive_data_via_net_buffer(
		arg->in.buffer_size,
		arg->in.buffer_id,
		1<<20,
		KEEP);
	channel_writer->write(bytes, arg->in.buffer_size);
	Genode::env()->heap()->free(bytes, arg->in.buffer_size);
}

void ZoogMonitor::Session_component::debug_write_channel_close(
	ZoogMonitor::Session::DispatchDebugWriteChannelClose *arg)
{
	channel_writer->close();
}

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
	case do_unmap_canvas:
		unmap_canvas(&d->un.unmap_canvas);
		break;
	case do_update_canvas:
		update_canvas(&d->un.update_canvas);
		break;
	case do_receive_ui_event:
		receive_ui_event(&d->un.receive_ui_event);
		break;
	case do_new_toplevel_viewport:
		new_toplevel_viewport(&d->un.new_toplevel_viewport);
		break;
	case do_verify_label:
		verify_label(&d->un.verify_label);
		break;
	case do_sublet_viewport:
		sublet_viewport(&d->un.sublet_viewport);
		break;
	case do_accept_viewport:
		accept_viewport(&d->un.accept_viewport);
		break;
	case do_repossess_viewport:
		repossess_viewport(&d->un.repossess_viewport);
		break;
	case do_get_deed_key:
		get_deed_key(&d->un.get_deed_key);
		break;
	case do_transfer_viewport:
		transfer_viewport(&d->un.transfer_viewport);
		break;
	case do_launch_application:
		launch_application(&d->un.launch_application);
		break;
	case do_allocate_memory:
		allocate_memory(d, &d->un.allocate_memory);
		break;
	case do_debug_write_channel_open:
		debug_write_channel_open(&d->un.debug_write_channel_open);
		break;
	case do_debug_write_channel_write:
		debug_write_channel_write(&d->un.debug_write_channel_write);
		break;
	case do_debug_write_channel_close:
		debug_write_channel_close(&d->un.debug_write_channel_close);
		break;
	}
//	PDBG("done opcode %d", d->opcode);
}


