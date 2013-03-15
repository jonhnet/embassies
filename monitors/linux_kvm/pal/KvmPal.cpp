#include <stdio.h>
#include "RandomSupply.h"
#include "pal_abi/pal_extensions.h"

#include "linux_kvm_protocol.h"
#include "malloc_factory_operator_new.h"
#include "ambient_malloc.h"
#include "math_util.h"

#include "KvmPal.h"
#include "LiteLib.h"

using namespace std;

void KvmPal::unimpl(const char *m)
{
   debug_logfile_append("stderr", m);
   exit();
}

KvmPal *KvmPal::g_pal = NULL;
// XaxDispatchTable has implied state; the callers don't have
// to pass the table in as an object. (This is because we never
// expect to have more than one. But hey, maybe in the interest
// of OO, we should just make that the protocol!)

KvmPal::KvmPal()
{
	lite_assert(g_pal==NULL);
	g_pal = this;

	_map_alarms();

	uint32_t *p, v;
	for (p = (uint32_t*) &zdt, v=0x00000f00;
		p<(uint32_t*)&((&zdt)[1]);
		p++, v++)
	{
		// make gaps in the zdt turn up as jumps to obviously-nonsense
		// pc values
		*p = v;
	}

	zdt.zoog_lookup_extension = lookup_extension;
    zdt.zoog_allocate_memory = allocate_memory;
    zdt.zoog_free_memory = free_memory;
    zdt.zoog_thread_create = thread_create;
    zdt.zoog_thread_exit = thread_exit;
    zdt.zoog_x86_set_segments = x86_set_segments;
    zdt.zoog_exit = exit;
    zdt.zoog_launch_application = launch_application;
	zdt.zoog_endorse_me = endorse_me;

	zdt.zoog_get_app_secret = get_app_secret;
	zdt.zoog_get_random = get_random;

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
	zdt.zoog_accept_viewport = accept_viewport;
	zdt.zoog_transfer_viewport = transfer_viewport;
	zdt.zoog_verify_label = verify_label;
	zdt.zoog_map_canvas = map_canvas;
	zdt.zoog_unmap_canvas = unmap_canvas;
	zdt.zoog_update_canvas = update_canvas;
	zdt.zoog_receive_ui_event = receive_ui_event;

	_set_up_idt();

	zoog_malloc_factory_init(&zmf, &zdt);
	malloc_factory_operator_new_init(zmf.mf);
	ambient_malloc_init(zmf.mf);
}

ZoogKvmCallPage *KvmPal::get_call_page()
{
	ZoogKvmCallPage *result;
	__asm__ volatile (
		// swap in our private gs...
		"mov " GDT_GS_PAL_SELECTOR_STR ",%%eax;"
		"mov %%eax,%%gs;"
		// ...snarf out pointer to call page...
		"mov %%gs:(0),%0;"
		// ...and put gs back how it was.
		"mov " GDT_GS_USER_SELECTOR_STR ",%%eax;"
		"mov %%eax,%%gs;"
		: "=r"(result)
		:
		: "%eax"
	);
	return result;
}

inline void KvmPal::call_monitor()
{
	asm("hlt;");
}

void *KvmPal::allocate_memory(size_t length)
{
	xa_allocate_memory *xa = (xa_allocate_memory *) get_call_page()->call_struct;
	xa->opcode = xp_allocate_memory;
	xa->in_length = length;
	call_monitor();
	return xa->out_pointer;
}

void KvmPal::free_memory(void *base, size_t length)
{
	xa_free_memory *xa = (xa_free_memory *) get_call_page()->call_struct;
	xa->opcode = xp_free_memory;
	xa->in_pointer = base;
	xa->in_length = length;
	call_monitor();
}

void KvmPal::get_ifconfig(XIPifconfig *ifconfig, int *inout_count)
{
	xa_get_ifconfig *xa = (xa_get_ifconfig *) get_call_page()->call_struct;
	xa->opcode = xp_get_ifconfig;
	call_monitor();
	uint32_t idx;
	for (idx=0; idx<xa->out_count && idx<((uint32_t) *inout_count); idx++)
	{
		ifconfig[idx] = xa->out_ifconfigs[idx];
	}
	*inout_count = idx;
}

ZoogNetBuffer *KvmPal::alloc_net_buffer(uint32_t payload_size)
{
	xa_alloc_net_buffer *xa = (xa_alloc_net_buffer *) get_call_page()->call_struct;
	xa->opcode = xp_alloc_net_buffer;
	xa->in_payload_size = payload_size;
	call_monitor();
	return xa->out_zoog_net_buffer;
}

void KvmPal::send_net_buffer(ZoogNetBuffer *znb, bool release)
{
	xa_send_net_buffer *xa = (xa_send_net_buffer *) get_call_page()->call_struct;
	xa->opcode = xp_send_net_buffer;
	xa->in_zoog_net_buffer = znb;
	xa->in_release = release;
	call_monitor();
}

ZoogNetBuffer *KvmPal::receive_net_buffer()
{
	xa_receive_net_buffer *xa = (xa_receive_net_buffer *) get_call_page()->call_struct;
	xa->opcode = xp_receive_net_buffer;
	call_monitor();
	return xa->out_zoog_net_buffer;
}

void KvmPal::free_net_buffer(ZoogNetBuffer *xnb)
{
	xa_free_net_buffer *xa = (xa_free_net_buffer *) get_call_page()->call_struct;
	xa->opcode = xp_free_net_buffer;
	xa->in_zoog_net_buffer = xnb;
	call_monitor();
}

bool KvmPal::thread_create(zoog_thread_start_f *func, void *stack)
{
	xa_thread_create *xa = (xa_thread_create *) get_call_page()->call_struct;
	xa->opcode = xp_thread_create;
	xa->in_func = func;
	xa->in_stack = stack;
	call_monitor();
	return xa->out_result;
}

void KvmPal::thread_exit()
{
	xa_thread_exit *xa = (xa_thread_exit *) get_call_page()->call_struct;
	xa->opcode = xp_thread_exit;
	call_monitor();
	lite_assert(false);
	// should not return
}

void KvmPal::exit()
{
	xa_exit *xa = (xa_exit *) get_call_page()->call_struct;
	xa->opcode = xp_exit;
	call_monitor();
	lite_assert(false);
	// should not return
}

void KvmPal::x86_set_segments(uint32_t fs, uint32_t gs)
{
	xa_x86_set_segments *xa = (xa_x86_set_segments *) get_call_page()->call_struct;
	xa->opcode = xp_x86_set_segments;
	xa->in_fs = fs;
	xa->in_gs = gs;
	call_monitor();
}

void KvmPal::endorse_me(
	ZPubKey* local_key, uint32_t cert_buffer_len, uint8_t* cert_buffer)
{
	ZoogKvmCallPage *cp = get_call_page();
	xa_endorse_me *xa = (xa_endorse_me *) cp->call_struct;
	xa->opcode = xp_endorse_me;
	lite_assert(local_key->size() <= cp->capacity-sizeof(*xa));
	xa->in_local_key_size = local_key->size();
	local_key->serialize(xa->in_local_key);
	call_monitor();
	lite_assert(xa->out_cert_size <= cert_buffer_len);
	lite_memcpy(cert_buffer, xa->out_cert, xa->out_cert_size);
}

uint32_t KvmPal::get_app_secret(
  uint32_t num_bytes_requested, uint8_t* buffer)
{
	ZoogKvmCallPage *cp = get_call_page();
	xa_get_app_secret *xa = (xa_get_app_secret *) cp->call_struct;
	xa->opcode = xp_get_app_secret;
	xa->in_bytes_requested = num_bytes_requested;
	call_monitor();
	lite_assert(xa->out_bytes_provided <= num_bytes_requested);
	lite_memcpy(buffer, xa->out_bytes, xa->out_bytes_provided);
	return xa->out_bytes_provided;
}

uint32_t KvmPal::get_random(uint32_t num_bytes_requested, uint8_t *buffer)
{
	ZoogKvmCallPage *cp = get_call_page();
	xa_get_random *xa = (xa_get_random *) cp->call_struct;
	xa->opcode = xp_get_random;
	xa->in_num_bytes_requested = num_bytes_requested;
	call_monitor();
	lite_assert(xa->out_bytes_provided <= num_bytes_requested);
	lite_memcpy(buffer, xa->out_bytes, xa->out_bytes_provided);
	return xa->out_bytes_provided;
}

void KvmPal::zutex_wait(ZutexWaitSpec *specs, uint32_t count)
{
	ZoogKvmCallPage *cp = get_call_page();
	xa_zutex_wait *xa = (xa_zutex_wait *) cp->call_struct;
	xa->opcode = xp_zutex_wait;

	// Too many specs for transmission on the call page. Bummer!
	// (This debug assert gets safety-enforced in monitor.)
	uint32_t spec_space = get_call_page()->capacity - sizeof(xa_zutex_wait);
	lite_assert(sizeof(ZutexWaitSpec)*count < spec_space);

	xa->in_count = count;
	lite_memcpy(xa->in_specs, specs, sizeof(ZutexWaitSpec)*count);
	call_monitor();
}

int KvmPal::zutex_wake(
	uint32_t *zutex,
	uint32_t match_val, 	// TODO this param probably dead code!
	Zutex_count n_wake,
	Zutex_count n_requeue,
	uint32_t *requeue_zutex,
	uint32_t requeue_match_val)
{
	ZoogKvmCallPage *cp = get_call_page();
	xa_zutex_wake *xa = (xa_zutex_wake *) cp->call_struct;
	xa->opcode = xp_zutex_wake;
	xa->in_zutex = zutex;
	xa->in_n_wake = n_wake;
	xa->in_n_requeue = n_requeue;
	xa->in_requeue_zutex = requeue_zutex;
	xa->in_requeue_match_val = requeue_match_val;
	call_monitor();
	return xa->out_number_woken;
}

ZoogHostAlarms *KvmPal::get_alarms()
{
	return g_pal->alarms;
}

void KvmPal::launch_application(
	SignedBinary *signed_binary)
{
	ZoogKvmCallPage *cp = get_call_page();
	xa_launch_application *xa = (xa_launch_application *) cp->call_struct;
	xa->opcode = xp_launch_application;
	xa->in_signed_binary = signed_binary;
	call_monitor();
}

void *KvmPal::lookup_extension(const char *name)
{
	if (lite_strcmp(name, DEBUG_LOGFILE_APPEND_NAME)==0)
	{
		return (void*) &debug_logfile_append;
	}
	else if (lite_strcmp(name, DEBUG_CREATE_TOPLEVEL_WINDOW_NAME)==0)
	{
		return (void*) debug_create_toplevel_window;
	}
	else
	{
		return NULL;
	}
}

void KvmPal::debug_logfile_append(const char *logfile_name, const char *message)
{
	ZoogKvmCallPage *cp = get_call_page();
	xa_extn_debug_logfile_append *xa = (xa_extn_debug_logfile_append *) cp->call_struct;
	xa->opcode = xp_extn_debug_logfile_append;
	int string_space = cp->capacity - sizeof(*xa);
	int message_len = lite_strlen(message);
	int truncated_message_len = min(message_len, string_space);
	xa->in_msg_len = truncated_message_len;
	lite_strncpy(xa->in_logfile_name, logfile_name, sizeof(xa->in_logfile_name));
	lite_strncpy(xa->in_msg_bytes, message, truncated_message_len);
	call_monitor();
}

ViewportID KvmPal::debug_create_toplevel_window()
{
	ZoogKvmCallPage *cp = get_call_page();
	xa_extn_debug_create_toplevel_window *xa = (xa_extn_debug_create_toplevel_window *) cp->call_struct;
	xa->opcode = xp_extn_debug_create_toplevel_window;
	call_monitor();
	return xa->out_viewport_id;
}

void KvmPal::idt_handler()
{
	//lite_assert(0);
	debug_logfile_append("stderr", "IDT invoked\n");
	lite_assert(0);
}

void KvmPal::_set_up_idt()
{
	ZoogKvmCallPage *cp = get_call_page();
	xa_debug_set_idt_handler *xa = (xa_debug_set_idt_handler *) cp->call_struct;
	xa->opcode = xp_debug_set_idt_handler;
	xa->in_handler = (uint32_t) &idt_handler;
	call_monitor();
}

void KvmPal::_map_alarms()
{
	ZoogKvmCallPage *cp = get_call_page();
	xa_internal_map_alarms *xa = (xa_internal_map_alarms *) cp->call_struct;
	xa->opcode = xp_internal_map_alarms;
	call_monitor();
	this->alarms = xa->out_alarms;
}

void *KvmPal::get_app_code_start()
{
	ZoogKvmCallPage *cp = get_call_page();
	xa_internal_find_app_code *xa = (xa_internal_find_app_code *) cp->call_struct;
	xa->opcode = xp_internal_find_app_code;
	call_monitor();
	return xa->out_app_code_start;
}

void KvmPal::get_time(
	uint64_t *out_time)
{
	ZoogKvmCallPage *cp = get_call_page();
	xa_get_time *xa = (xa_get_time *) cp->call_struct;
	xa->opcode = xp_get_time;
	call_monitor();
	*out_time = xa->out_time;
}

void KvmPal::set_clock_alarm(
	uint64_t in_time)
{
	ZoogKvmCallPage *cp = get_call_page();
	xa_set_clock_alarm *xa = (xa_set_clock_alarm *) cp->call_struct;
	xa->opcode = xp_set_clock_alarm;
	xa->in_time = in_time;
	call_monitor();
}

void KvmPal::sublet_viewport(
	ViewportID tenant_viewport,
	ZRectangle *rectangle,
	ViewportID *out_landlord_viewport,
	Deed *out_deed)
{
	ZoogKvmCallPage *cp = get_call_page();
	xa_sublet_viewport *xa = (xa_sublet_viewport *) cp->call_struct;
	xa->opcode = xp_sublet_viewport;
	xa->in_tenant_viewport = tenant_viewport;
	xa->in_rectangle = *rectangle;
	call_monitor();
	*out_landlord_viewport = xa->out_landlord_viewport;
	*out_deed = xa->out_deed;
}

void KvmPal::repossess_viewport(
	ViewportID landlord_viewport)
{
	ZoogKvmCallPage *cp = get_call_page();
	xa_repossess_viewport *xa = (xa_repossess_viewport *) cp->call_struct;
	xa->opcode = xp_repossess_viewport;
	xa->in_landlord_viewport = landlord_viewport;
	call_monitor();
}

void KvmPal::get_deed_key(
	ViewportID landlord_viewport,
	DeedKey *out_deed_key)
{
	ZoogKvmCallPage *cp = get_call_page();
	xa_get_deed_key *xa = (xa_get_deed_key *) cp->call_struct;
	xa->opcode = xp_get_deed_key;
	xa->in_landlord_viewport = landlord_viewport;
	call_monitor();
	*out_deed_key = xa->out_deed_key;
}

void KvmPal::accept_viewport(
	Deed *deed,
	ViewportID *out_tenant_viewport,
	DeedKey *out_deed_key)
{
	ZoogKvmCallPage *cp = get_call_page();
	xa_accept_viewport *xa = (xa_accept_viewport *) cp->call_struct;
	xa->opcode = xp_accept_viewport;
	xa->in_deed = *deed;
	call_monitor();
	*out_tenant_viewport = xa->out_tenant_viewport;
	*out_deed_key = xa->out_deed_key;
}

void KvmPal::transfer_viewport(
	ViewportID tenant_viewport,
	Deed *out_deed)
{
	ZoogKvmCallPage *cp = get_call_page();
	xa_transfer_viewport *xa = (xa_transfer_viewport *) cp->call_struct;
	xa->opcode = xp_transfer_viewport;
	xa->in_tenant_viewport = tenant_viewport;
	call_monitor();
	*out_deed = xa->out_deed;
}

bool KvmPal::verify_label(ZCertChain* chain)
{
	ZoogKvmCallPage *cp = get_call_page();
	xa_verify_label *xa = (xa_verify_label *) cp->call_struct;
	xa->opcode = xp_verify_label;
	lite_assert(chain->size() <= cp->capacity - sizeof(*xa));
	xa->in_chain_size_bytes = chain->size();
	chain->serialize(xa->in_chain);
	call_monitor();
	return xa->out_result;
}

void KvmPal::map_canvas(
	ViewportID viewport_id,
	PixelFormat *known_formats,
	int num_formats,
	ZCanvas *out_canvas)
{
	ZoogKvmCallPage *cp = get_call_page();
	xa_map_canvas *xa = (xa_map_canvas *) cp->call_struct;
	xa->opcode = xp_map_canvas;
	xa->in_viewport_id = viewport_id;
	int xa_capacity = sizeof(xa->in_known_formats)/sizeof(xa->in_known_formats[0]);
	if (num_formats > xa_capacity)
	{
		num_formats = xa_capacity;
	}
	xa->in_num_formats = num_formats;
	lite_memcpy(xa->in_known_formats, known_formats, sizeof(PixelFormat)*num_formats);
	call_monitor();
	lite_memcpy(out_canvas, &xa->out_canvas, sizeof(ZCanvas));
}

void KvmPal::unmap_canvas(ZCanvasID canvas_id)
{
	ZoogKvmCallPage *cp = get_call_page();
	xa_unmap_canvas *xa = (xa_unmap_canvas *) cp->call_struct;
	xa->opcode = xp_unmap_canvas;
	xa->in_canvas_id = canvas_id;
	call_monitor();
}

void KvmPal::update_canvas(ZCanvasID canvas_id,
	ZRectangle *rectangle)
{
	ZoogKvmCallPage *cp = get_call_page();
	xa_update_canvas *xa = (xa_update_canvas *) cp->call_struct;
	xa->opcode = xp_update_canvas;
	xa->in_canvas_id = canvas_id;
	xa->in_rectangle = *rectangle;
	call_monitor();
}

void KvmPal::receive_ui_event(ZoogUIEvent *out_event)
{
	ZoogKvmCallPage *cp = get_call_page();
	xa_receive_ui_event *xa = (xa_receive_ui_event *) cp->call_struct;
	xa->opcode = xp_receive_ui_event;
	call_monitor();
	lite_memcpy(out_event, &xa->out_event, sizeof(ZoogUIEvent));
}

