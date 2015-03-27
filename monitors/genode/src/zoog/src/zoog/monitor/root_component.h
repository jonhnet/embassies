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
#include <base/process.h>
#include <ram_session/connection.h>
#include <cpu_session/connection.h>
#include <cap_session/connection.h>

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
#include "RandomSeed.h"
#include "RandomSupply.h"
#include "common/SyncFactory_Genode.h"
#include "ZKeyPair.h"
#include "zoog_network_order.h"
#include "boot_block.h"
#include "session_component.h"
#include "ChannelWriter.h"

namespace ZoogMonitor {
class Root_component : public Genode::Root_component<Session_component>
{
	protected:
		MallocFactory *mf;
		SyncFactory_Genode sf;
		GBlitProvider gblit_provider;
		Timer::Connection timer;
		RandomSeed random_seed;
		RandomSupply random_supply;
		BlitterManager blitter_manager;
		Zoog_genode_process_id _next_zoog_process_id;
		ZPubKey* zoog_ca_root_key;

		Lock launch_lock;
		List<ZoogMonitor::Session_component> embroyonic_sessions;
		List<BootBlock> boot_blocks;

		DebugFlags _dbg_flags;
		Genode::Allocator_avl _tx_block_alloc;
		Nic::Connection _nic;
		XIPifconfig _prototype_ifconfig;
		ZoogMonitor::NetworkThread _network_thread;
		ChannelWriter channel_writer;

		////////////////////////////////////////////////////////////
		static XIPifconfig construct_ifconfig(uint32_t low_octet);
		void _do_launches();

		ZoogMonitor::Session_component *_create_session(const char *args);

		void init_zoog_ca_root_key();

	public:

		Root_component(Genode::Rpc_entrypoint *ep,
					   Genode::Allocator *allocator);

		ZPubKey *get_zoog_ca_root_key();

		void queue_boot_block(BootBlock* boot_block);

		void queue_session(ZoogMonitor::Session_component* session);

		DebugFlags* dbg_flags() { return &_dbg_flags; }
};

}
