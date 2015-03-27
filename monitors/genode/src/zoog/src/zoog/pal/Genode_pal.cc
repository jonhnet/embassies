/*
 * \brief  Genode_pal implements Zoog PAL ABI on Genode
 * \author Jon Howell
 * \date   2011.12.24
 */

#include <base/thread.h>
#include <util/misc_math.h>
#include <base/x86_segments.h>

#include "CoreWriter.h"
#include "Genode_pal.h"
#include "abort.h"
#include "corefile.h"
#include "zoog_qsort.h"
#include "CoreWriter.h"
#include "zoog_network_order.h"

#ifndef ZOOG_ROOT
#error ZOOG_ROOT not defined on compile command line
#endif

#define DBG_HACK_BOOT_PATH_RAW ZOOG_ROOT "/toolchains/linux_elf/paltest/build/paltest.signed"
//#define DBG_HACK_BOOT_PATH_RAW ZOOG_ROOT "/toolchains/linux_elf/elf_loader/build/elf_loader.vendor_a.signed"

using namespace Genode;

static GenodePAL *g_pal = NULL;

enum { STACK_SIZE = 4096 };

//////////////////////////////////////////////////////////////////////////////

class CoreDumpTimer : public Thread<16384> {
private:
	GenodePAL *pal;
public:
	CoreDumpTimer(GenodePAL *pal) : pal(pal) {}
	void entry() { pal->do_timer_thread(); }
};

//////////////////////////////////////////////////////////////////////////////

GenodePAL::PalLocker::PalLocker(GenodePAL *pal, int line)
	: pal(pal),
	  line(line)
{
//	PDBG("PalLocker(this 0x%08x, pal 0x%08x)\n", (int) this, (int) pal);
	pal->lock.lock();
//	PDBG("Line %d has the lock", line);
	this->t = pal->thread_table.lookup_me();
	t->set_core_state();
}

GenodePAL::PalLocker::~PalLocker()
{
	t->clear_core_state();
//	PDBG("Line %d releases the lock", line);
	pal->lock.unlock();
}

//////////////////////////////////////////////////////////////////////////////

GenodePAL::GenodePAL()
	: mem_allocator(&sync_factory, false),
	  mf(standard_malloc_factory_init()),
	  zutex_table(mf, &sync_factory),
	  host_alarms(&zutex_table),
	  zoog_alarm_thread(&zutex_table, &host_alarms, &sync_factory),
	  monitor_alarm_thread(&host_alarms, &zoog_monitor),
	  main_thread(&thread_table, &sync_factory),
	  debug_server(this, &zoog_monitor),
	  canvas_downsampler_manager(mf),
	  next_allocation_location(0x10000000)
{
	PDBG("dbg: GenodePAL ctor\n");
	if (g_pal!=NULL) {
		throw Exception();
	}
	g_pal = this;

//	PDBG("GenodePAL: start-gate init\n");
	// Silly start-gate mechanism -- wait for monitor to tell us when to go.
	Signal_receiver signal_receiver;
	Signal_context start_gate_receive_context;
	Signal_context_capability start_gate_sigh_cap =
		signal_receiver.manage(&start_gate_receive_context);
	zoog_monitor.start_gate_sigh(start_gate_sigh_cap);
//	PDBG("GenodePAL: start-gate wait\n");
	signal_receiver.wait_for_signal();

//	PDBG("GenodePAL: start-gate complete\n");
	for (int i=0; i<MAX_DISPATCH_SLOTS; i++) { dispatch_slot_used[i] = false; }
	Dataspace_capability dispatch_region_cap
		= zoog_monitor.map_dispatch_region(MAX_DISPATCH_SLOTS);
	Dataspace_client dispatch_region_dataspace(dispatch_region_cap);
	dispatch_region = (ZoogMonitor::Session::Dispatch *)
		env()->rm_session()->attach(dispatch_region_dataspace);

	// channel_writer cannot be initialized until g_pal exists and
	// dispatch region is mapped, since it makes a monitor rpc.
	ZoogNetBuffer* channel_writer_znb = alloc_net_buffer(1<<20);
	ZoogMonitor::Session::Zoog_genode_net_buffer_id channel_writer_buffer_id =
		g_pal->net_buffer_table.lookup(channel_writer_znb, "channel_writer");
	channel_writer = new ChannelWriterClient(
		this, channel_writer_znb, channel_writer_buffer_id);

#if 0
		timer.msleep(1000);
	channel_writer->write_one("test", "bar\n");
		timer.msleep(1000);
	channel_writer->write_one("test", "bazbooooooger\n");
		timer.msleep(1000);

	uint8_t buf[32768];
	memset(buf, 0x7, sizeof(buf));
	channel_writer->open("data", sizeof(buf));
	channel_writer->write(buf, sizeof(buf));
	channel_writer->close();
		timer.msleep(1000);

	channel_writer->write_one("test", "a little string\n");

	for (int i=0; i<100; i++)
	{
		timer.msleep(1000);
		channel_writer->write_one("test", "tick\n");
	}
#endif

	Dataspace_capability boot_block_dataspace_cap = zoog_monitor.get_boot_block();
	Dataspace_client boot_block_dataspace(boot_block_dataspace_cap);
	printf("PAL: boot_block_dataspace.size()==%08x\n",
		boot_block_dataspace.size());

	uint32_t *p;
	uint32_t i=0;
	for (p=(uint32_t*) &zdt; p<(uint32_t*) &((&zdt)[1]); p++, i++)
	{
		*p = 0x33333300 | i;
	}

	zdt.zoog_lookup_extension = lookup_extension;
	zdt.zoog_allocate_memory = allocate_memory;
	zdt.zoog_free_memory = free_memory;
	zdt.zoog_thread_create = thread_create;
	zdt.zoog_thread_exit = thread_exit;
	zdt.zoog_x86_set_segments = x86_set_segments;
	/*
	zdt.zoog_exit = (zf_zoog_exit) unimpl;
	*/
	zdt.zoog_launch_application = launch_application;
	zdt.zoog_zutex_wait = zutex_wait;
	zdt.zoog_zutex_wake = zutex_wake;
	zdt.zoog_get_alarms = get_alarms;

	zdt.zoog_get_ifconfig = get_ifconfig;
	zdt.zoog_alloc_net_buffer = alloc_net_buffer;
	zdt.zoog_send_net_buffer = send_net_buffer;
	zdt.zoog_receive_net_buffer = receive_net_buffer;
	zdt.zoog_free_net_buffer = free_net_buffer;

	zdt.zoog_get_time = get_time;
	zdt.zoog_set_clock_alarm = set_clock_alarm;

	zdt.zoog_sublet_viewport = sublet_viewport;
	zdt.zoog_repossess_viewport = repossess_viewport;
	zdt.zoog_get_deed_key = get_deed_key;

	// tenant calls
	zdt.zoog_accept_viewport = accept_viewport;
	zdt.zoog_transfer_viewport = transfer_viewport;
	zdt.zoog_verify_label = verify_label;
	zdt.zoog_map_canvas = map_canvas;
	zdt.zoog_unmap_canvas = unmap_canvas;
	zdt.zoog_update_canvas = update_canvas;
	zdt.zoog_receive_ui_event = receive_ui_event;

#if 1
	boot_block = env()->rm_session()->attach(boot_block_dataspace);
#else
	// why am I hardcoding the attach-at!? Maybe this was before we had
	// debug_mmap.
	boot_block = env()->rm_session()->attach_at(boot_block_dataspace, 0x120000);
#endif
//	PDBG("That address has");
//	PDBG("byte %02x", ((uint8_t*)0x54c7b)[0]);
//	PDBG("boot_block = %08x, end = %08x",
//		(uint32_t) boot_block,
//		(uint32_t) (boot_block+boot_block_dataspace.size()));

#define HACK_LOADER_PATH ../../toolchains/linux_elf/elf_loader/build/elf_loader_zguest.raw
	{
		char buf[256];
		snprintf(buf, sizeof(buf), "L %s 0x%08x\n",
			DBG_HACK_BOOT_PATH_RAW,
			(uint32_t) boot_block);
		debug_logfile_append("mmap", buf);
	}

	linked_list_init(&extra_ranges, mf);
	// record it for core file
	Range *bb_range = new Range((uint32_t) boot_block, (uint32_t) boot_block+boot_block_dataspace.size());
	linked_list_insert_head(&extra_ranges, bb_range);

	Dataspace_capability app_stack_cap =
		env()->ram_session()->alloc(STACK_SIZE);
	uint8_t *app_stack =
		env()->rm_session()->attach(app_stack_cap);

	Range *as_range = new Range((uint32_t) app_stack, (uint32_t) app_stack+STACK_SIZE);
	linked_list_insert_head(&extra_ranges, as_range);

//	PDBG("test core, before things go south");
//	dump_core();

//	core_dump_timer = new CoreDumpTimer(this);
//	core_dump_timer->start();

	launch(boot_block, app_stack+STACK_SIZE, &zdt);
}

void GenodePAL::debug_logfile_append(const char *logfile_name, const char *message)
{
	PWRN("%s: %s", logfile_name, message);
//	PDBG("g_pal: %p cw %p", g_pal, g_pal->channel_writer);
	g_pal->channel_writer->write_one(logfile_name, message);
}

uint32_t GenodePAL::debug_get_link_mtu()
{
	return 10220;
}

ViewportID GenodePAL::debug_create_toplevel_window()
{
	PalLocker lock(g_pal, __LINE__);

	Dispatcher d(g_pal, ZoogMonitor::Session::do_new_toplevel_viewport);
	d.rpc();
	return d.d()->un.new_toplevel_viewport.out.viewport_id;
}

void *GenodePAL::lookup_extension(const char *name)
{
//	PDBG("GenodePAL::lookup_extension");
	if (strcmp(name, DEBUG_LOGFILE_APPEND_NAME)==0)
	{
		return (void*) debug_logfile_append;
	}
	else if (strcmp(name, DEBUG_GET_LINK_MTU_NAME)==0)
	{
		return (void*) debug_get_link_mtu;
	}
	else if (strcmp(name, DEBUG_CREATE_TOPLEVEL_WINDOW_NAME)==0)
	{
		return (void*) debug_create_toplevel_window;
	}
	else {
		return NULL;
	}
}

void GenodePAL::do_timer_thread()
{
	PDBG("Core dump timer starts");
	int i;
	for (i=0; i<12; i++) {
		PDBG("Core dump tick");
		timer.msleep(1000);
	}

//	char *stomp = (char*) 0x54c78;
//	PDBG("Before %c", stomp[0]);
//	lite_strcpy(stomp, "Flabbulous");
//	PDBG("After %c==%c", stomp[0], 'F');

	dump_core();

//	PDBG("And at that address:\n");
//	int j;
//	for (j=-16; j<48; j++)
//	{
//		uint8_t *addr = (uint8_t*) 0x54c7b+j;
//		PWRN("%08x: %02x\n", (uint32_t) addr, addr[0]);
//	}
}

static int zoog_thread_compar(const void *a, const void *b)
{
	ZoogThread **za = (ZoogThread **) a;
	ZoogThread **zb = (ZoogThread **) b;
	return ((int)(*za)->core_thread_id()) - ((int)(*zb)->core_thread_id());
}

void GenodePAL::dump_core()
{
	PDBG("Dumping core...");
	lock.lock();
	PDBG("  (dump_core) PAL lock acquired");
	CoreFile core;
	corefile_init(&core, mf);

	// nb we've locked PAL, so thread table won't change.
	int num_threads = thread_table.get_count();
	ZoogThread **threads = (ZoogThread **)
		mf_malloc(mf, sizeof(ZoogThread*) * num_threads);

	thread_table.enumerate(threads, num_threads);
	for (int threadi=0; threadi<num_threads; threadi++)
	{
		// tracking down a NULL ZoogThread.
		PDBG("before sort: ZoogThread %d/%d %p\n", threadi, num_threads, threads[threadi]);
	}

	// sort by id, so they come up sane in gdb
	zoog_qsort(threads, num_threads, sizeof(ZoogThread*), zoog_thread_compar);

	for (int threadi=0; threadi<num_threads; threadi++)
	{
		ZoogThread *zt = threads[threadi];
		if (!zt->core_state_valid())
		{
			// Huh. thread is blocked or running elsewhere.
			// Capture *something* of him.
			Thread_state state;
			env()->cpu_session()->state(zt->cap(), &state);
			zt->fudge_core_state(state.ip, state.sp);
		}
		Core_x86_registers *regs = zt->get_core_regs();
		PDBG("thread %d: ip %08x sp %08x bp %08x",
			zt->core_thread_id(), regs->ip, regs->sp, regs->bp);
		corefile_add_thread(&core, regs, zt->core_thread_id());
	}
	mf_free(mf, threads);

	// add the non-memory-allocated ranges
	LinkedListIterator lli;
	for (ll_start(&extra_ranges, &lli);
		ll_has_more(&lli);
		ll_advance(&lli))
	{
		Range *r = (Range*) ll_read(&lli);
//		PDBG("Segment at %p size %x", (void*) r->start, r->size());
		corefile_add_segment(&core, r->start, (void*) r->start, r->size());
	}

	// add the memory-allocated ranges
	Range range;
	UserObjIfc *dummy;
	dummy = mem_allocator.first_range(&range);
	while (dummy!=NULL)
	{
//		PDBG("Segment at %p size %x", (void*) range.start, range.size());
		size_t rounded_up_size = ((range.size()-1) | 0x0fff) + 1;
		corefile_add_segment(&core,
			range.start, (void*) range.start, rounded_up_size);
		dummy = mem_allocator.next_range(range, &range);
	}

	// add the outstanding network buffers
	LinkedList znbs;
	linked_list_init(&znbs, mf);
	PDBG("znbs LL is %p", &znbs);
	net_buffer_table.enumerate_znbs(&znbs);
	while (true)
	{
		PDBG("Removing head from LL %p", &znbs);
		ZoogNetBuffer *znb = (ZoogNetBuffer *) linked_list_remove_head(&znbs);
		if (znb==NULL)
			{ break; }
		// round things to page boundaries; corefile assumes it, and
		// presumably L4/Genode's dataspace mapping must make it true.
		uint32_t start = ((uint32_t) znb) & ~0xfff;
		uint32_t end = ((uint32_t) znb) + sizeof(*znb) + znb->capacity;
		end = ((end-1) & ~0xfff)+0x1000;
		PDBG("Adding ZNB core segment %p len %x", (void*) start, end-start);
		corefile_add_segment(&core, start, (void*) start, end-start);
	}
	
	// TODO wire this through from the build script variables
	corefile_set_bootblock_info(&core, boot_block, DBG_HACK_BOOT_PATH_RAW);

	uint32_t size = corefile_compute_size(&core);
	PDBG("Computed core size will be %d bytes", size);

	{
		CoreWriter core_writer(channel_writer, size);
		corefile_write_custom(&core, CoreWriter::static_write, NULL, &core_writer);
	}
	lock.unlock();
	PDBG("Dumped core.");
}

void GenodePAL::_grow_memory(size_t length)
{
	/* assumes lock is locked */
	size_t request_length = max(length, (size_t) MIN_ALLOCATION_UNIT);

	PDBG("GenodePAL::_grow_memory %d\n", request_length);
	Dispatcher d(g_pal, ZoogMonitor::Session::do_allocate_memory);
	d.d()->un.allocate_memory.in.length = request_length;
	d.rpc();
	Dataspace_capability ds_cap = d.d()->out_dataspace_capability;

//	Dataspace_capability ds_cap = env()->ram_session()->alloc(request_length);
	PDBG("GenodePAL::_grow_memory successful\n");
	Dataspace_client ds_client(ds_cap);
	addr_t backing;
	while (true)
	{
		try {
			backing = env()->rm_session()->attach(
				ds_client, 0, 0, true, (addr_t) next_allocation_location);
			break;
		} catch (Rm_session::Attach_failed) {
			PDBG("Attach_failed; advancing %08x", MIN_ALLOCATION_UNIT);
			next_allocation_location += MIN_ALLOCATION_UNIT;
			continue;
		}
	}
	next_allocation_location += request_length;
	PDBG("Attached more memory at %08lx", backing);
	Range new_range(backing, backing+ds_client.size());
	g_pal->mem_allocator.create_empty_range(new_range);
}

void *GenodePAL::allocate_memory(size_t length)
{
//	PDBG("length %08x", length);

	// Round all allocations to pages, so that CoalescingAllocator
	// naturally returns allocations on page boundaries.
	size_t rounded_length = ((length-1) & ~0x0fff) + 0x1000;
	if ((length & 0x0fff)!=0)
	{
		PDBG("Rounding length 0x%x to 0x%x\n", length, rounded_length);
	}
	length = rounded_length;

	Range out_range;

	PalLocker lock(g_pal, __LINE__);

	bool rc;
	rc = g_pal->mem_allocator.allocate_range(length, &g_pal->dummy_user_obj, &out_range);
	if (!rc)
	{
		g_pal->_grow_memory(length);
		rc = g_pal->mem_allocator.allocate_range(length, &g_pal->dummy_user_obj, &out_range);
		if (!rc) {
			abort("memory allocation failure");
		}
	}
	memset((void*) out_range.start, 0, out_range.size());
	void *result = (void*) out_range.start;

	return result;
}

void GenodePAL::free_memory(void *base, size_t length)
{
	PalLocker lock(g_pal, __LINE__);

	// Hah, we weren't ever detaching any of the memory we alloc'd, either. Lovely.
	//PDBG("free_memory %08x ... %08x", (uint32_t) base, ((uint32_t) base)+length);
	Range range((uint32_t)base, ((uint32_t)base)+length);
	g_pal->mem_allocator.free_range(range);
}


bool GenodePAL::thread_create(zoog_thread_start_f *func, void *stack)
{
	PalLocker lock(g_pal, __LINE__);

	ZoogThread *zt = new ZoogThread((void*) func, stack, &g_pal->thread_table, &g_pal->sync_factory);
	zt->start();

	return true;
}

void GenodePAL::thread_exit(void)
{
	PDBG("thread_exit start");
	g_pal->lock.lock();	// lock thread table
	ZoogThread *t = g_pal->thread_table.remove_me();
	g_pal->lock.unlock();
	env()->cpu_session()->kill_thread(t->cap());
	PDBG("thread_exit done");
}

#if 0 // dead (debug) code
static uint32_t _get_gs_selector(void)
{
	uint32_t _result;
	asm volatile (
		"movl	%%gs, %%eax\n\t"
		"movl	%%eax, %0"
		: "=m"(_result)
		: "m"(_result)
		: "%eax"
	);
	return _result;
}

static uint32_t _deref_gs(void)
{
	int gsval;
	__asm__ __volatile__ (
		"mov	%%gs:(0),%0"
		: "=r" (gsval)
		:
	);
	return gsval;
}
#endif

static void _set_gs_selector(uint32_t new_gs)
{
	asm volatile (
		"movl	%0, %%eax\n\t"
		"movl	%%eax, %%gs"
		: "=m"(new_gs)
		: "m"(new_gs)
		: "%eax"
	);
}

void GenodePAL::x86_set_segments(uint32_t fs, uint32_t gs)
{
//	uint32_t saved_gs_selector = _get_gs_selector();
//	PDBG("x86_set_segments(fs %08x, gs %08x)\n", fs, gs);
//	PDBG("before gs selector %08x\n", _get_gs_selector());
//	PDBG("       ...derefs to %08x\n", _deref_gs());
	Set_x86_segments sxs;
	sxs.platform_set_x86_segments(fs, gs);
//	PDBG("baz!");
	_set_gs_selector(0x63);
		// X86_ZTLS from contrib/kernel/src/glue/v4-x86/x32/config.h
//	uint32_t derefed = _deref_gs();
//	_set_gs_selector(saved_gs_selector);	// reenable PDBG
//	PDBG("bang!");
//	PDBG(";after gs selector %08x\n", _get_gs_selector());
//	PDBG("       ...derefs to %08x\n", derefed);
//	PDBG("       0(%08x) = %08x\n", gs, ((uint32_t*)gs)[0]);
//	PDBG("       without help, gs derefs to %08x\n", _deref_gs());
//	int *x=0; *x=7;
}

void GenodePAL::launch_application(SignedBinary *signed_binary)
{
	uint32_t signed_binary_size =
		sizeof(SignedBinary)
		+Z_NTOHG(signed_binary->cert_len)
		+Z_NTOHG(signed_binary->binary_len);
	lite_assert(signed_binary_size < (5<<20));	// rough sanity check -- 5MB
	ZoogNetBuffer *znb = alloc_net_buffer(signed_binary_size);
		// NB abuse of the net buffer mechanism for communicating biggish
		// data up to monitor.
	memcpy(ZNB_DATA(znb), signed_binary, signed_binary_size);

	PalLocker lock(g_pal, __LINE__);

	ZoogMonitor::Session::Zoog_genode_net_buffer_id buffer_id
		= g_pal->net_buffer_table.lookup(znb, "launch_application");

	Dispatcher d(g_pal, ZoogMonitor::Session::do_launch_application);
	d.d()->un.launch_application.in.buffer_size = signed_binary_size;
	d.d()->un.launch_application.in.buffer_id = buffer_id;
	d.rpc();

	g_pal->net_buffer_table.remove(buffer_id);

	PDBG("launch_application (parent) done.\n");
}

void GenodePAL::zutex_wait(ZutexWaitSpec *specs, uint32_t count)
{
	PalLocker plock(g_pal, __LINE__);

	g_pal->lock.unlock();	// have to release pal lock to wait!
		// TODO is it safe? What invariant guarantees that the thread
		// and its related data structures won't be wonked while we're
		// unlocked? (I guess the fact that threads can only exit themselves,
		// and here we are in the only thread that could call thread_exit.)
	g_pal->zutex_table.wait(specs, count, plock.get_thread()->waiter());
	g_pal->lock.lock();	// put it back so ~PalLocker can unlock again
}

int GenodePAL::zutex_wake(
    uint32_t *zutex,
    uint32_t match_val, 
    Zutex_count n_wake,
    Zutex_count n_requeue,
    uint32_t *requeue_zutex,
    uint32_t requeue_match_val)
{
	lite_assert(n_requeue==0);
	PalLocker lock(g_pal, __LINE__);

	return g_pal->zutex_table.wake((uint32_t) zutex, n_wake);
}

ZoogHostAlarms *GenodePAL::get_alarms()
{
	return g_pal->host_alarms.get_zoog_host_alarms();
}

void GenodePAL::get_ifconfig(XIPifconfig *ifconfig, int *inout_count)
{
	PalLocker lock(g_pal, __LINE__);

	Dispatcher d(g_pal, ZoogMonitor::Session::do_get_ifconfig);
	d.rpc();

	ZoogMonitor::Session::DispatchGetIfconfig *reply = &d.d()->un.get_ifconfig;
	int i;
	for (i=0; i<(int)reply->out.num_ifconfigs && i<*inout_count; i++)
	{
		ifconfig[i] = reply->out.ifconfigs[i];
	}
	*inout_count = i;
}

ZoogNetBuffer *GenodePAL::alloc_net_buffer(uint32_t payload_size)
{
	PalLocker lock(g_pal, __LINE__);

	Dataspace_capability buffer_cap
		= g_pal->zoog_monitor.alloc_net_buffer(payload_size);
	ZoogNetBuffer *result = g_pal->net_buffer_table.insert(buffer_cap);

	return result;
}

void GenodePAL::send_net_buffer(ZoogNetBuffer *znb, bool release)
{
	PalLocker lock(g_pal, __LINE__);

	ZoogMonitor::Session::Zoog_genode_net_buffer_id buffer_id
		= g_pal->net_buffer_table.lookup(znb, "send_net_buffer");

	Dispatcher d(g_pal, ZoogMonitor::Session::do_send_net_buffer);
	d.d()->un.send_net_buffer.in.buffer_id = buffer_id;
	d.d()->un.send_net_buffer.in.release = release;
	d.rpc();

	// at this point, if release is true, the znb memory has gone
	// away, because the server has yanked it back!
	// so we have to name the buffer by id.
	if (release) {
		g_pal->net_buffer_table.remove(buffer_id);
	}
}

ZoogNetBuffer *GenodePAL::receive_net_buffer()
{
	PalLocker lock(g_pal, __LINE__);

	ZoogMonitor::Session::Receive_net_buffer_reply reply = g_pal->zoog_monitor.receive_net_buffer();
	ZoogNetBuffer *result;
	if (reply.valid)
	{
		ZoogNetBuffer *znb = g_pal->net_buffer_table.insert(reply.cap);
//		PDBG("returning %d-byte packet", znb->capacity);
		result = znb;
	} else {
		// TODO I think this is getting called too often by the zoog app code, but eh.
//		PDBG("returning NULL");
		result = NULL;
	}

	return result;
}

void GenodePAL::free_net_buffer(ZoogNetBuffer *znb)
{
	PalLocker lock(g_pal, __LINE__);

	ZoogMonitor::Session::Zoog_genode_net_buffer_id buffer_id
		= g_pal->net_buffer_table.lookup(znb, "free_net_buffer");

	Dispatcher d(g_pal, ZoogMonitor::Session::do_free_net_buffer);
	d.d()->un.free_net_buffer.in.buffer_id = buffer_id;
	d.rpc();

	g_pal->net_buffer_table.remove(buffer_id);
}

void GenodePAL::get_time(uint64_t *out_time)
{
	*out_time = g_pal->zoog_alarm_thread.get_time();
}

void GenodePAL::set_clock_alarm(uint64_t scheduled_time)
{
	g_pal->zoog_alarm_thread.reset_alarm(scheduled_time);
}

void GenodePAL::sublet_viewport(
	ViewportID tenant_viewport,
	ZRectangle *rectangle,
	ViewportID *out_landlord_viewport,
	Deed *out_deed)
{
	PalLocker lock(g_pal, __LINE__);

	Dispatcher d(g_pal, ZoogMonitor::Session::do_sublet_viewport);
	d.d()->un.sublet_viewport.in.tenant_viewport = tenant_viewport;
	d.d()->un.sublet_viewport.in.rectangle = *rectangle;
	d.rpc();

	*out_landlord_viewport = d.d()->un.sublet_viewport.out.landlord_viewport;
	*out_deed = d.d()->un.sublet_viewport.out.deed;
}

void GenodePAL::accept_viewport(
	Deed *deed,
	ViewportID *out_tenant_viewport,
	DeedKey *out_deed_key)
{
	PalLocker lock(g_pal, __LINE__);

	Dispatcher d(g_pal, ZoogMonitor::Session::do_accept_viewport);
	d.d()->un.accept_viewport.in.deed = *deed;
	d.rpc();

	*out_tenant_viewport = d.d()->un.accept_viewport.out.tenant_viewport;
	*out_deed_key = d.d()->un.accept_viewport.out.deed_key;
}

void GenodePAL::repossess_viewport(
	ViewportID landlord_viewport)
{
	PalLocker lock(g_pal, __LINE__);

	Dispatcher d(g_pal, ZoogMonitor::Session::do_repossess_viewport);
	d.d()->un.repossess_viewport.in.landlord_viewport = landlord_viewport;
	d.rpc();
}

void GenodePAL::get_deed_key(
	ViewportID landlord_viewport,
	DeedKey *out_deed_key)
{
	PalLocker lock(g_pal, __LINE__);

	Dispatcher d(g_pal, ZoogMonitor::Session::do_get_deed_key);
	d.d()->un.get_deed_key.in.landlord_viewport = landlord_viewport;
	d.rpc();

	*out_deed_key = d.d()->un.get_deed_key.out.deed_key;
}

void GenodePAL::transfer_viewport(
	ViewportID tenant_viewport,
	Deed *out_deed)
{
	PalLocker lock(g_pal, __LINE__);

	Dispatcher d(g_pal, ZoogMonitor::Session::do_transfer_viewport);
	d.d()->un.transfer_viewport.in.tenant_viewport = tenant_viewport;
	d.rpc();

	*out_deed = d.d()->un.transfer_viewport.out.deed;
}
	
bool GenodePAL::verify_label(ZCertChain* chain)
{
	// Sleazily recycling net buffers to transmit big data to monitor.
	// Can't lock yet, because I'm using alloc_net_buffer, which locks.

	uint32_t chain_size = chain->size();
	lite_assert(chain_size < (1<<16));	// rough sanity check
	ZoogNetBuffer *znb = alloc_net_buffer(chain_size);
		// NB abuse of the net buffer mechanism for communicating biggish
		// data up to monitor.
	chain->serialize((uint8_t*) ZNB_DATA(znb));

	PalLocker lock(g_pal, __LINE__);

	ZoogMonitor::Session::Zoog_genode_net_buffer_id buffer_id
		= g_pal->net_buffer_table.lookup(znb, "verify_label");

	Dispatcher d(g_pal, ZoogMonitor::Session::do_verify_label);
	d.d()->un.verify_label.in.buffer_size = chain_size;
	d.d()->un.verify_label.in.buffer_id = buffer_id;
	d.rpc();

	g_pal->net_buffer_table.remove(buffer_id);

	bool result = d.d()->un.verify_label.out.result;

	PDBG("verify_label: %d", result);
	return result;
}

void GenodePAL::map_canvas(
	ViewportID viewport_id, PixelFormat *known_formats, int num_formats,
	ZCanvas *out_canvas)
{
	PalLocker lock(g_pal, __LINE__);

	lite_assert(num_formats==1);
	lite_assert(known_formats[0] == zoog_pixel_format_truecolor24);
	PDBG("map_canvas 1\n");
	ZoogMonitor::Session::MapCanvasReply mcr =
		g_pal->zoog_monitor.map_canvas(viewport_id);
	*out_canvas = mcr.canvas;
	PDBG("map_canvas 2\n");
	g_pal->canvas_downsampler_manager.create(
		out_canvas,
		env()->rm_session()->attach(mcr.framebuffer_dataspace_cap));
	PDBG("map_canvas 3\n");
}
	
void GenodePAL::update_canvas(ZCanvasID canvas_id, ZRectangle* rectangle)
{
	PalLocker lock(g_pal, __LINE__);

	CanvasDownsampler *cd = g_pal->canvas_downsampler_manager.lookup(canvas_id);
	cd->update(rectangle->x, rectangle->y, rectangle->width, rectangle->height);

	Dispatcher d(g_pal, ZoogMonitor::Session::do_update_canvas);
	d.d()->un.update_canvas.in.canvas_id = canvas_id;
	d.d()->un.update_canvas.in.rect = *rectangle;
	d.rpc();
}
	
void GenodePAL::receive_ui_event(ZoogUIEvent *out_event)
{
	PalLocker lock(g_pal, __LINE__);

	Dispatcher d(g_pal, ZoogMonitor::Session::do_receive_ui_event);
	d.rpc();
	*out_event = d.d()->un.receive_ui_event.out.evt;
}

void GenodePAL::unmap_canvas(ZCanvasID canvas_id)
{
	PalLocker lock(g_pal, __LINE__);

	Dispatcher d(g_pal, ZoogMonitor::Session::do_unmap_canvas);
	d.d()->un.unmap_canvas.in.canvas_id = canvas_id;
	d.rpc();
	lite_assert(false);	// unimpl -- need to detach dataspace. Oh, maybe monitor server can do that for me.
}

void GenodePAL::unimpl(void)
{
	PalLocker lock(g_pal, __LINE__);
		// want to get this thread labeled in coredump

	g_pal->lock.unlock();
	abort("GenodePAL::unimpl");
	// should not occur
	g_pal->lock.lock();
}

void GenodePAL::idle() {
	while (1) {
		//zoog_monitor.dbg_ping();
		timer.msleep(1000);
	}
}

void GenodePAL::launch(uint8_t *eip, uint8_t *esp, ZoogDispatchTable_v1 *zdt) {
	PDBG("eip %08x esp %08x zdt %08x", (uint32_t) eip, (uint32_t) esp, (uint32_t) zdt);
	asm(
		"movl	%0,%%esp;"
		"movl	%1,%%ebx;"
		"nop;"
		"movl	%2,%%eax;"
		"jmp	*%%eax;"
		:	// out
		:	"r"(esp), "r"(zdt) // in
		, "r"(eip)
		);
}

ZoogMonitor::Session::Dispatch* GenodePAL::assign_dispatch_slot(int* out_idx)
{
	int idx;
	for (idx=0; idx<MAX_DISPATCH_SLOTS; idx++) {
		if (!dispatch_slot_used[idx])
		{
			dispatch_slot_used[idx] = true;
			break;
		}
	}
	lite_assert(idx < MAX_DISPATCH_SLOTS);	// ran out of slots!
	*out_idx = idx;
	return &dispatch_region[idx];
}
