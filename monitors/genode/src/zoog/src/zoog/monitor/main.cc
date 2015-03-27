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

#include "root_component.h"

using namespace Genode;

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
	Capability<Typed_root<ZoogMonitor::Session_component> > monitor_cap =
		ep.manage(&zoog_monitor_root);
	env()->parent()->announce(monitor_cap);

	ZBlitter zblitter;

	// Now start the first pal instance
//	Parent_capability parent_cap(monitor_cap);
#if 0
	const char* label = "zoog_pal";
	long priority = 1;	// TODO ??
	long quota = 1<<20;	// start with 1MB?
	Rom_connection _binary_rom(label, "zoog_pal_0");
	Ram_connection ram(label);
	Cpu_connection cpu(label, priority);
	Rm_connection  rm;
	ram.ref_account(env()->ram_session_cap());
	env()->ram_session()->transfer_quota(ram.cap(), quota);
	Process initial_process(
		_binary_rom.dataspace(),
		ram.cap(),
		cpu.cap(),
		rm.cap(),
		parent_cap(),
		"initial_process",
		NULL);
#endif

	/* We are done with this and only act upon client requests now. */
	PDBG("Main completed; calling sleep_forever\n");
	sleep_forever();

	return 0;
}
