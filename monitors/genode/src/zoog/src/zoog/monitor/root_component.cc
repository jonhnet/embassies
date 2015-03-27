#include "root_component.h"

extern char _binary_zoog_root_key_start;
extern char _binary_zoog_root_key_end;

ZoogMonitor::Root_component::Root_component(Genode::Rpc_entrypoint *ep,
			   Genode::Allocator *allocator)
: Genode::Root_component<Session_component>(ep, allocator),
  mf(standard_malloc_factory_init()),
  gblit_provider(&timer),
  random_supply(random_seed.get_seed_bytes()),
  blitter_manager(&gblit_provider,
	mf,
	&sf,
	&random_supply),
  _next_zoog_process_id(3),
  _tx_block_alloc(env()->heap()),
  _nic(&_tx_block_alloc),
  _prototype_ifconfig(construct_ifconfig(254)),
  _network_thread(&_nic, &_prototype_ifconfig, &_dbg_flags, &timer)
{
	PDBG("Creating root component.");
	init_zoog_ca_root_key();
	queue_boot_block(BootBlock::get_initial_boot_block());
	_network_thread.start();
}

XIPifconfig ZoogMonitor::Root_component::construct_ifconfig(uint32_t low_octet)
{
	lite_assert(low_octet > 2);
	lite_assert(low_octet < 255);
	XIPifconfig result;
	result.local_interface = literal_ipv4xip(10,200,0,low_octet);
	result.gateway = literal_ipv4xip(10,200,0,1);
	result.netmask = literal_ipv4xip(255,255,255,0);
	return result;
}

ZoogMonitor::Session_component *ZoogMonitor::Root_component::_create_session(const char *args)
{
	Zoog_genode_process_id this_zoog_process_id = _next_zoog_process_id;
	_next_zoog_process_id += 1;
	PDBG("creating ZoogMonitor session, id %d.", this_zoog_process_id);
	XIPifconfig this_ifconfig(construct_ifconfig(this_zoog_process_id));
	return new (md_alloc())
		Session_component(
			this,
			&blitter_manager,
			&timer,
			this_zoog_process_id,
			&this_ifconfig,
			&_network_thread,
			&channel_writer);
}

void ZoogMonitor::Root_component::init_zoog_ca_root_key()
{
	ZKeyPair* rootKeyPair = new ZKeyPair(
		(uint8_t*) &_binary_zoog_root_key_start,
		(&_binary_zoog_root_key_start) - (&_binary_zoog_root_key_end));
	zoog_ca_root_key = new ZPubKey(rootKeyPair->getPubKey());
	delete rootKeyPair;
}

ZPubKey *ZoogMonitor::Root_component::get_zoog_ca_root_key()
{
	return zoog_ca_root_key;
}

void ZoogMonitor::Root_component::_do_launches()
{
	while (true)
	{
		ZoogMonitor::Session_component* session =
			embroyonic_sessions.first();
		BootBlock* boot_block =
			boot_blocks.first();
		if (session==NULL)
		{
			PDBG("No more sessions.\n");
			break;
		}
		if (boot_block==NULL)
		{
			PDBG("No more boot blocks.\n");
			break;
		}
		PDBG("Assigning a boot block to a waiting session.\n");
		embroyonic_sessions.remove(session);
		boot_blocks.remove(boot_block);
		session->launch(boot_block);
	}
}

void ZoogMonitor::Root_component::queue_boot_block(BootBlock* boot_block)
{
	PDBG("queue_boot_block\n");
	launch_lock.lock();
	boot_blocks.insert(boot_block);
	_do_launches();
	launch_lock.unlock();
}

void ZoogMonitor::Root_component::queue_session(ZoogMonitor::Session_component* session)
{
	PDBG("queue_session\n");
	launch_lock.lock();
	embroyonic_sessions.insert(session);
	_do_launches();
	launch_lock.unlock();
}
