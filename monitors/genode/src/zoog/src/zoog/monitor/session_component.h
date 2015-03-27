/*
 * \brief  Zoog Monitor -- trusted components of Zoog ABI
 * \author Jon Howell
 * \date   2011-12-22
 */

#pragma once

#include "ZBlitter.h"

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

#include <zoog_monitor_session/zoog_monitor_session.h>

#include "NetworkThread.h"
#include "standard_malloc_factory.h"
#include "xax_network_utils.h"
#include "hash_table.h"
#include "AllocatedBuffer.h"
#include "UIEventQueue.h"
#include "RandomSeed.h"
#include "RandomSupply.h"
#include "common/SyncFactory_Genode.h"
#include "ZKeyPair.h"
#include "zoog_network_order.h"
#include "ChannelWriter.h"

#include "zoog_genode_process_id.h"

namespace ZoogMonitor {

class BootBlock;
class Root_component;

// TODO separate these by the client we gave them to. (gulp)
class Session_component
	: public Genode::Rpc_object<Session>,
	  public BlitPalIfc,
	  public List<ZoogMonitor::Session_component>::Element
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

	////////////////////////////////////////////////////////////
	// Private members
	////////////////////////////////////////////////////////////

	ChannelWriter* channel_writer;	// disk log output multiplexer
	HashTable _allocated_buffer_table;
	Session::Zoog_genode_net_buffer_id _next_id;

	BootBlock* _boot_block;
	Zoog_genode_process_id zoog_process_id;
	XIPifconfig _ifconfig;
	Signal_transmitter network_receive_signal_transmitter;
	Signal_transmitter ui_event_signal_transmitter;
	Signal_transmitter core_dump_invoke_signal_transmitter;
	Signal_transmitter start_gate_signal_transmitter;

	ZoogMonitor::Session::Dispatch *dispatch_region;
	uint32_t dispatch_region_count;

	MallocFactory *mf;
	SyncFactory_Genode sf;

	UIEventTransmitter uiet;
	UIEventQueue uieq;
	Root_component* root_component;
	BlitterManager* blitter_manager;
	ZPubKey *pub_key;
	NetworkTerminus networkTerminus;

	////////////////////////////////////////////////////////////
	// Private methods
	////////////////////////////////////////////////////////////

	AllocatedBuffer *_retrieve_buffer(Session::Zoog_genode_net_buffer_id buffer_id);
	enum ReleaseBuffer { KEEP, RELEASE };
	uint8_t* _receive_data_via_net_buffer(
		uint32_t claimed_size,
		Zoog_genode_net_buffer_id buffer_id,
		uint32_t size_sanity_check,
		ReleaseBuffer release_buffer_req);
	void release_buffer(AllocatedBuffer *allocated_buffer);
	static XIPifconfig construct_ifconfig(uint32_t low_octet);
	static void dbg_print_buffer_ids(void *user_obj, void *a);

	void get_ifconfig(ZoogMonitor::Session::DispatchGetIfconfig *arg);
	void send_net_buffer(ZoogMonitor::Session::DispatchSendNetBuffer *arg);
	void free_net_buffer(ZoogMonitor::Session::DispatchFreeNetBuffer *arg);
	void unmap_canvas(ZoogMonitor::Session::DispatchUnmapCanvas *arg);
	void update_canvas(ZoogMonitor::Session::DispatchUpdateCanvas *arg);
	void receive_ui_event(ZoogMonitor::Session::DispatchReceiveUIEvent *arg);
	void new_toplevel_viewport(ZoogMonitor::Session::DispatchNewToplevelViewport *arg);
	void verify_label(ZoogMonitor::Session::DispatchVerifyLabel *arg);
	void sublet_viewport(ZoogMonitor::Session::DispatchSubletViewport *arg);
	void accept_viewport(ZoogMonitor::Session::DispatchAcceptViewport *arg);
	void repossess_viewport(ZoogMonitor::Session::DispatchRepossessViewport *arg);
	void get_deed_key(ZoogMonitor::Session::DispatchGetDeedKey *arg);
	void transfer_viewport(ZoogMonitor::Session::DispatchTransferViewport *arg);
	void launch_application(ZoogMonitor::Session::DispatchLaunchApplication *arg);
	void allocate_memory(ZoogMonitor::Session::Dispatch *d, ZoogMonitor::Session::DispatchAllocateMemory *arg);
	void debug_write_channel_open(ZoogMonitor::Session::DispatchDebugWriteChannelOpen *arg);
	void debug_write_channel_write(ZoogMonitor::Session::DispatchDebugWriteChannelWrite *arg);
	void debug_write_channel_close(ZoogMonitor::Session::DispatchDebugWriteChannelClose *arg);

public:
	Session_component(
		Root_component* root_component,
		BlitterManager* blitter_manager,
		Timer::Connection *timer,
		Zoog_genode_process_id zoog_process_id,
		XIPifconfig *ifconfig,
		NetworkThread* network_thread,
		ChannelWriter* channel_writer);
	~Session_component();

	void launch(BootBlock* boot_block);
	Dataspace_capability map_dispatch_region(uint32_t region_count);
	Dataspace_capability get_boot_block();
	Dataspace_capability alloc_net_buffer(uint32_t payload_size);
	ZoogMonitor::Session::Receive_net_buffer_reply receive_net_buffer();
	void event_receive_sighs(Signal_context_capability network_cap, Signal_context_capability ui_cap);
	void core_dump_invoke_sigh(Signal_context_capability cap);
	void start_gate_sigh(Signal_context_capability cap);
	MapCanvasReply map_canvas(ViewportID viewport_id);
	void dispatch_message(uint32_t index);

#if 0	// debug rpc-level dispatches.
	Rpc_exception_code dispatch(int opcode, Ipc_istream &is, Ipc_ostream &os)
	{
		PDBG("rpc dispatch %d\n", opcode);
		Rpc_exception_code result;
		result = Rpc_object::dispatch(opcode, is, os);
		return result;
	}
#endif
	

public:
	// BlitPalIfc
	UIEventDeliveryIfc *get_uidelivery() { return &uieq; }
	ZPubKey *get_pub_key() { return pub_key; }
	void debug_remark(const char *msg) { PDBG("Blitter sez '%s'", msg); }
};
}
