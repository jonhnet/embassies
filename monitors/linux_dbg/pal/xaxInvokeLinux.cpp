#include <sys/types.h>
#include <sys/times.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <linux/unistd.h>
#include <asm/ldt.h>
#include <sched.h>
#include <time.h>
#include <alloca.h>
#include <signal.h>

#include "ClockAlarmThread.h"
#include "MonitorAlarmThread.h"
#include "malloc_factory_operator_new.h"
#include "pal_abi/pal_abi.h"
#include "pal_abi/pal_extensions.h"
#include "linux_dbg_port_protocol.h"
#include "cheesylock.h"
#include "memtracker.h"
#include "malloc_trace.h"
#include "raw_clone.h"
#include "xaxInvokeLinux.h"
#include "abstract_mmap.h"
#include "cheesymalloc.h"
#include "cheesy_malloc_factory.h"
#include "profiler.h"
#include "gsfree_syscall.h"
#include "gsfree_lib.h"
#include "xpbtable.h"
#include "xax_util.h"
#include "cheesy_snprintf.h"
#include "ZLCEmitGsfree.h"
#include "SyncFactory_GsfreeFutex.h"
#include "ambient_malloc.h"
#include "CallCounts.h"
#include "pal_thread.h"
#include "TunIDAllocator.h"

#define DBG_MSG(s) safewrite_str(s)

//#define TRACE_PAGES_TOUCHED 1

//////////////////////////////////////////////////////////////////////////////

void _xil_zutex_wait_inner(ZutexWaitSpec *specs, uint32_t count);

void _debug_logfile_append_gsinner(const char *logfile_name, const char *message);

//////////////////////////////////////////////////////////////////////////////

#define ARCH_i386 1

#define XIL_TRACE_ALLOC 1

#if 0
#define CALL_COUNT_ENABLED 1
#define CALL_COUNT(field) \
	{ \
		PalState *pal = &g_pal_state; \
		pal->call_counts->tally(offsetof(ZoogDispatchTable_v1, field)/4); \
	}
#else
#define CALL_COUNT_ENABLED 0
#define CALL_COUNT(field) {}
#endif

//////////////////////////////////////////////////////////////////////////////

#define N_PIPES 20

typedef struct {
	CheesyLock cheesyLock;
	int pipe_fd[N_PIPES];
	int capacity;
} XaxPipePool;

typedef struct {
	AbstractMmapIfc ami;
} ConcreteMmapPAL;

typedef struct {
	ConcreteMmapPAL cmp;
	CheesyMallocArena cma;
	CheesyMallocFactory cmf;
} MFArena;

struct s_pal_state	/* defines typedef PalState */
{
public:
	uint32_t process_id;
	int tunid;

	XaxPipePool pipe_pool;
	ZoogDispatchTable_v1 xil_dispatch_table;
	XPBTable *xpb_table;

	MFArena main_mf_arena;
	MallocFactory *mf;
	MFArena profiler_mf_arena;
	MallocFactory *profiler_mf;

	ClockAlarmThread *clock_alarm_thread;
	MonitorAlarmThread *monitor_alarm_thread;
	SyncFactory *sf;

	MemTracker *mt[tl_COUNT];
	MallocTraceFuncs mtf, *mtfp;

	ZLCEmit *ze;

	Profiler profiler;
	time_t start_time;

	HostAlarms host_alarms;

#if CALL_COUNT_ENABLED
	CallCounts *call_counts;
#endif // CALL_COUNT_ENABLED
};

//////////////////////////////////////////////////////////////////////////////

// Inside PAL, where gs is 0, we can't re-enter our own xdt (hence
// we don't use concrete_mmap_xdt). But we can just call mmap, as long
// as it doesn't fail (and set errno). :v)

void *concrete_mmap_pal_mmap(AbstractMmapIfc *ami, size_t length)
{
	return mmap(NULL, length,
		PROT_READ|PROT_WRITE|PROT_EXEC,
		MAP_PRIVATE|MAP_ANONYMOUS,
		-1,
		0);
}

void concrete_mmap_pal_munmap(AbstractMmapIfc *ami, void *addr, size_t length)
{
	munmap(addr, length);
}

void concrete_mmap_pal_init(ConcreteMmapPAL *cmp)
{
	cmp->ami.mmap_f = concrete_mmap_pal_mmap;
	cmp->ami.munmap_f = concrete_mmap_pal_munmap;
}

//////////////////////////////////////////////////////////////////////////////

void xil_x86_set_segments(uint32_t fs, uint32_t gs);
void xil_x86_set_segments_internal(uint32_t fs, uint32_t gs);
int _internal_thread_get_id(void);

int _gsfree_getpid(void)
{
	return _gsfree_syscall(__NR_getpid);
}

int _gsfree_gettid(void)
{
	return _gsfree_syscall(__NR_gettid);
}

#define SOCKOP_socket		1
#define SOCKOP_bind		2
#define SOCKOP_connect		3
#define SOCKOP_listen		4
#define SOCKOP_accept		5
#define SOCKOP_getsockname	6
#define SOCKOP_getpeername	7
#define SOCKOP_socketpair	8
#define SOCKOP_send		9
#define SOCKOP_recv		10
#define SOCKOP_sendto		11
#define SOCKOP_recvfrom		12
#define SOCKOP_shutdown		13
#define SOCKOP_setsockopt	14
#define SOCKOP_getsockopt	15
#define SOCKOP_sendmsg		16
#define SOCKOP_recvmsg		17

int gsfree_socketcall(uint32_t syscallnum, uint32_t subcode, ...)
{
	uint8_t *argstruct = (uint8_t*) &subcode;
	argstruct += sizeof(subcode);
	return _gsfree_syscall(syscallnum, subcode, argstruct);
}

typedef struct {
	uint32_t caller_gs_base;
} XILThreadContext;

void xtc_init(XILThreadContext *xtc)
{
	xtc->caller_gs_base = get_gs_base();
}

#define PROTECT_GS_START() \
	{ XILThreadContext __xtc; \
	xtc_init(&__xtc); \
	xil_x86_set_segments_internal(0, 0);
	// Note use of unbalanced {'s to enforce bracketed use in program
	// text. It'll make for horrible compiler errors, but at least
	// they'll be errors.
	// NB also the assumption that the caller puts his gs base
	// at %gs:(0). libc does. Good luck! Well, this won't work for Windows DPI
	// apps. Rats. This isn't a generalizable solution. Really
	// need a %gs-free library.
#define PROTECT_GS_END() \
	xil_x86_set_segments_internal(0, __xtc.caller_gs_base); }

void tinyprintf(const char *fn, uint32_t val)
{
	char buf[200];
	strcpy(buf, fn);
	strcat(buf, ": ");
	hexstr(buf+strlen(buf), val);
	strcat(buf, "\n");
	DBG_MSG(buf);
}

static void xax_pipe_pool_init(XaxPipePool *pool)
{
	cheesy_lock_init(&pool->cheesyLock);
	pool->capacity = N_PIPES;
	int i;
	for (i=0; i<pool->capacity; i++)
	{
		pool->pipe_fd[i] = -1;
	}
}

static int _xax_alloc_pipe(PalState *pal)
{
	cheesy_lock_acquire(&pal->pipe_pool.cheesyLock);

	int i;

	// take an existing pipe
	int result = -1;
	for (i=0; i<pal->pipe_pool.capacity; i++)
	{
		if (pal->pipe_pool.pipe_fd[i] != -1)
		{
			result = pal->pipe_pool.pipe_fd[i];
			pal->pipe_pool.pipe_fd[i] = -1;
			break;
		}
	}

	cheesy_lock_release(&pal->pipe_pool.cheesyLock);

	if (result == -1)
	{
		int rc;

		//int fd = socket(PF_UNIX, SOCK_DGRAM, 0);
		int fd = gsfree_socketcall(
				__NR_socketcall, SOCKOP_socket, PF_UNIX, SOCK_DGRAM, 0);

		gsfree_assert(fd>=0);

		struct sockaddr_un srcaddr;
		srcaddr.sun_family = PF_UNIX;
		strcpy(srcaddr.sun_path, "/tmp/xax_client_");
		char buf[10];
		strcat(srcaddr.sun_path, hexstr(buf, _gsfree_getpid()));

		while (1)
		{
			rc = gsfree_socketcall(
				__NR_socketcall, SOCKOP_bind, fd, (struct sockaddr*) &srcaddr, sizeof(srcaddr));
			if (rc==0)
			{
				break;
			}
			if (rc== -EADDRINUSE)
			{
				// goofball unary search. :v) Should only need two "unique" names.
				gsfree_assert(strlen(srcaddr.sun_path) < sizeof(srcaddr.sun_path)-2);
				strcat(srcaddr.sun_path, "+");
				continue;
			}
//			write(2, "errno: ", strlen("errno: "));
//			char buf[17];
//			hexstr(buf, errno);
//			write(2, buf, strlen(buf));
//			write(2, "\n", strlen("\n"));
			gsfree_assert(0);
		}
		gsfree_assert(rc==0);

		struct sockaddr_un dstaddr;
		dstaddr.sun_family = PF_UNIX;
		cheesy_snprintf(dstaddr.sun_path, sizeof(dstaddr.sun_path),
			"%s:%d", XAX_PORT_RENDEZVOUS, pal->tunid);
		dstaddr.sun_path[sizeof(dstaddr.sun_path)-1] = '\0';

		rc = gsfree_socketcall(__NR_socketcall, SOCKOP_connect, fd, (struct sockaddr*) &dstaddr, sizeof(dstaddr));
		if (rc!=0)
		{
			DBG_MSG("Cannot connect: xaxPortMonitor dead?\n");
		}
		gsfree_assert(rc==0);

		{
			DBG_MSG("new pipe\n");
		}

		result = fd;
	}

	return result;
}

static void _xax_free_pipe(PalState *pal, int fd)
{
	cheesy_lock_acquire(&pal->pipe_pool.cheesyLock);
	
	int i;

	for (i=0; i<pal->pipe_pool.capacity; i++)
	{
		if (pal->pipe_pool.pipe_fd[i] == -1)
		{
			pal->pipe_pool.pipe_fd[i] = fd;
			break;
		}
	}
	gsfree_assert(i<pal->pipe_pool.capacity);	// we allocated more frames than we had room to store in pool! Now the pool leaks.

	cheesy_lock_release(&pal->pipe_pool.cheesyLock);
}

void _xax_port_rpc(PalState *pal, XpReqHeader *req, XpReplyHeader *reply_hdr, int reply_capacity)
{
	int _xax_port_fd = _xax_alloc_pipe(pal);

	int rc;
	rc = _gsfree_syscall(__NR_write, _xax_port_fd, (void*) req, req->message_len);
	if (!(rc==(int)req->message_len))
	{
		fprintf(stderr, "rc %d ml %d\n", rc, req->message_len);
	}
	gsfree_assert(rc==(int)req->message_len);

	while (1)
	{
		rc = _gsfree_syscall(__NR_read, _xax_port_fd, (void*) reply_hdr, reply_capacity);
		if (rc<0)
		{
			if (rc == -EINTR)
			{
				continue;
			}
		}
		gsfree_assert(rc>=(int)sizeof(XpReplyHeader));

		gsfree_assert(rc == (int)reply_hdr->message_len);
		break;
	}

	_xax_free_pipe(pal, _xax_port_fd);
}

PalState g_pal_state;

void *xil_allocate_memory(size_t length)
{
	CALL_COUNT(zoog_allocate_memory);
	void *rc;
	PROTECT_GS_START();

#define GUARD 0
#define GUARD_SIZE	(0x1000)
/*
	Careful. If we free guard pages based on free() base/length,
	we'll break apps that use munmap to free subregions of memory
	(I'm looking at YOU, JavaScript Collector.cpp.)
	But if we don't free them, we blow through VM address space too
	quickly.
	Thus, if we want to use this for big programs, we'd need to actually
	track allocations so we know when to free guard pages.
*/
#if GUARD
	size_t rounded_length = (length + 0x0fff) & 0xfffff000;
	size_t guard_length = rounded_length + 2*GUARD_SIZE;
#else
	size_t guard_length = length;
#endif
	rc = mmap(NULL, guard_length,
		PROT_READ|PROT_WRITE|PROT_EXEC,
		MAP_PRIVATE|MAP_ANONYMOUS,
		-1,
		0);

#if GUARD
	int procrc;
	procrc = mprotect(rc, GUARD_SIZE, PROT_NONE);
	gsfree_assert(procrc==0);
	procrc = mprotect(rc+guard_length-GUARD_SIZE, GUARD_SIZE, PROT_NONE);
	gsfree_assert(procrc==0);
	rc += GUARD_SIZE;
#endif

#if DEBUG_BROKEN_MMAP
	fprintf(stderr, "(pal) mmap(A %08x, L %08x) => %08x\n", base, length, rc);
#endif // DEBUG_BROKEN_MMAP

#if XIL_TRACE_ALLOC
	PalState *pal = &g_pal_state;
	if (pal->mt[tl_xil]!=NULL && rc!=MAP_FAILED)
	{
		mt_trace_alloc(pal->mt[tl_xil], (uint32_t) rc, length);
	}
#endif

	PROTECT_GS_END();
	return rc;
}

void xil_free_memory(void *base, size_t length)
{
	CALL_COUNT(zoog_free_memory);
	int rc;
	PROTECT_GS_START();

#if GUARD
	void *guard_base = base - 0x1000;
	size_t rounded_length = (length + 0x0fff) & 0xfffff000;
	size_t guard_length = rounded_length + 0x2000;
#else
	void *guard_base = base;
	size_t guard_length = length;
#endif
	rc = munmap(guard_base, guard_length);
#if XIL_TRACE_ALLOC
	PalState *pal = &g_pal_state;
	if (pal->mt[tl_xil]!=NULL && rc==0)
	{
		mt_trace_free(pal->mt[tl_xil], (uint32_t) base);
	}
#endif
	gsfree_assert(rc==0);

	PROTECT_GS_END();
}

#define DECLARE_REQUEST(NN,varname) \
	XpReq##NN varname; \
	varname.message_len = sizeof(varname); \
	varname.opcode = varname.OPCODE; \
	varname.process_id = pal->process_id; \

#define INIT_VARIABLE_REQUEST(req,msg_len) \
	req->message_len = msg_len; \
	req->opcode = req->OPCODE; \
	req->process_id = pal->process_id; \

extern void xil_alarm_poll(uint32_t *known_sequence_numbers, uint32_t *new_sequence_numbers)
{
	PalState *pal = &g_pal_state;

	DECLARE_REQUEST(EventPoll, req);
	XpReplyEventPoll reply;
	memcpy(req.known_sequence_numbers, known_sequence_numbers,
		sizeof(req.known_sequence_numbers));
	_xax_port_rpc(pal, &req, &reply, sizeof(reply));
	memcpy(new_sequence_numbers, reply.new_sequence_numbers,
		sizeof(reply.new_sequence_numbers));
}

extern void xil_get_ifconfig(XIPifconfig *ifconfig, int *inout_count)
{
	CALL_COUNT(zoog_get_ifconfig);
	PalState *pal = &g_pal_state;

	PROTECT_GS_START();
	DECLARE_REQUEST(GetIfconfig, req);
	XpReplyGetIfconfig reply;
	_xax_port_rpc(pal, &req, &reply, sizeof(reply));
	int i;
	for (i=0; i<*inout_count && i<reply.count; i++)
	{
		memcpy(&ifconfig[i], &reply.ifconfig_out[i], sizeof(ifconfig[i]));
	}
	*inout_count = i;
	PROTECT_GS_END();
}

XaxPortBuffer *decode_xpb_reply(PalState *pal, XpBufferReply *reply)
{
	XaxPortBuffer *xpb = NULL;
	switch (reply->type)
	{
	case use_open_buffer:
		xpb = pal->xpb_table->lookup(reply->map_spec.open_buffer_handle);
		break;
	case no_packet_ready:
		xpb = NULL;
		break;
	case use_map_file_spec:
		xpb = pal->xpb_table->insert(
			new MMapMapping(&reply->map_spec.map_file_spec),
			reply->map_spec.map_file_spec.deprecates_old_handle);
		break;
	case use_shmget_key:
		xpb = pal->xpb_table->insert(
			new ShmMapping(&reply->map_spec.shmget_key_spec),
			reply->map_spec.shmget_key_spec.deprecates_old_handle);
		break;
	default:
		gsfree_assert(false);
		xpb = NULL;
	}
	return xpb;
}

ZoogNetBuffer *decode_buffer_reply(PalState *pal, XpBufferReply *reply)
{
	XaxPortBuffer *xpb = decode_xpb_reply(pal, reply);
	return xpb==NULL ? NULL : &xpb->un.znb.znb_header;
}

void decode_canvas_reply(PalState *pal, XpBufferReply *reply, ZCanvas *out_canvas)
{
	XaxPortBuffer *xpb = decode_xpb_reply(pal, reply);
	if (xpb==NULL)
	{
		out_canvas->pixel_format = zoog_pixel_format_invalid;
		out_canvas->width = 0;
		out_canvas->height = 0;
		out_canvas->data = NULL;
	}
	else
	{
		*out_canvas = xpb->un.canvas.canvas;
		out_canvas->data = xpb->un.canvas._data;
		gsfree_assert(out_canvas->data == ((uint8_t*)&xpb->un.canvas)+sizeof(ZCanvas));
	}
}

ZoogNetBuffer *xil_alloc_net_buffer(uint32_t payload_size)
{
	CALL_COUNT(zoog_alloc_net_buffer);
	PalState *pal = &g_pal_state;
	ZoogNetBuffer *xnb;

	PROTECT_GS_START();
	DECLARE_REQUEST(XNBAlloc, req);
	req.thread_id = _internal_thread_get_id();
	req.payload_size = payload_size;
	XpReplyXNBAlloc reply;
	_xax_port_rpc(pal, &req, &reply, sizeof(reply));
	xnb = decode_buffer_reply(pal, &reply);
	gsfree_assert(xnb!=NULL);
#if XIL_TRACE_ALLOC
	if (pal->mt[tl_xil]!=NULL && xnb!=NULL)
	{
		mt_trace_alloc(pal->mt[tl_xil], (uint32_t) xnb, xnb->capacity);
	}
#endif
	PROTECT_GS_END();

	return xnb;
}

XaxPortBuffer *_xpb_from_xnb(ZoogNetBuffer *xnb)
{
	// Since we always hand out XNBs that live inside XPBs,
	// pointer subtraction is a safe way to get back to our
	// XPB structure.
	return (XaxPortBuffer*)
		(((uint8_t*)xnb)-(offsetof(XaxPortBuffer, un)));
}

enum { DISCARD_THRESHOLD = 1<<20 };

void xil_free_net_buffer(ZoogNetBuffer *xnb)
{
	CALL_COUNT(zoog_free_net_buffer);
	PalState *pal = &g_pal_state;
	XaxPortBuffer *xpb = _xpb_from_xnb(xnb);

	PROTECT_GS_START();
#if XIL_TRACE_ALLOC
	if (pal->mt[tl_xil]!=NULL)
	{
		mt_trace_free(pal->mt[tl_xil], (uint32_t) xnb);
	}
#endif

	DECLARE_REQUEST(XNBRelease, req);
	req.open_buffer_handle = xpb->buffer_handle;
	req.discarded_at_pal = false; // (xpb->capacity > DISCARD_THRESHOLD);
	XpReplyXNBRelease reply;
	_xax_port_rpc(pal, &req, &reply, sizeof(reply));

	if (req.discarded_at_pal)
	{
		pal->xpb_table->discard(xpb);
	}

	PROTECT_GS_END();
}

void xil_send_net_buffer(ZoogNetBuffer *xnb, bool release)
{
	CALL_COUNT(zoog_send_net_buffer);
	PalState *pal = &g_pal_state;
	XaxPortBuffer *xpb = _xpb_from_xnb(xnb);

	PROTECT_GS_START();
#if XIL_TRACE_ALLOC
	if (release && pal->mt[tl_xil]!=NULL)
	{
		mt_trace_free(pal->mt[tl_xil], (uint32_t) xnb);
	}
#endif
	DECLARE_REQUEST(XNBSend, req);
	req.open_buffer_handle = xpb->buffer_handle;
	req.release = release;
	XpReplyXNBSend reply;
	_xax_port_rpc(pal, &req, &reply, sizeof(reply));
	PROTECT_GS_END();
}

ZoogNetBuffer *xil_receive_net_buffer(void)
{
	CALL_COUNT(zoog_receive_net_buffer);
	PalState *pal = &g_pal_state;
	ZoogNetBuffer *xnb;

	PROTECT_GS_START();
	DECLARE_REQUEST(XNBRecv, req);
	req.thread_id = _internal_thread_get_id();
	XpReplyXNBRecv reply;
	_xax_port_rpc(pal, &req, &reply, sizeof(reply));
	xnb = decode_buffer_reply(pal, &reply);
#if XIL_TRACE_ALLOC
	if (pal->mt[tl_xil]!=NULL && xnb!=NULL)
	{
		mt_trace_alloc(pal->mt[tl_xil], (uint32_t) xnb, xnb->capacity);
	}
#endif
	PROTECT_GS_END();

	return xnb;
}

bool xil_thread_create(zoog_thread_start_f *func, void *stack)
{
	CALL_COUNT(zoog_thread_create);
	int rc;
/*
	XfThreadCreate *xf =
		(XfThreadCreate *) _xax_alloc_io_frame(sizeof(XfThreadCreate));
	xf->frame.opcode = XAX_OP_THREAD_CREATE;
	xf->func = func;
	xf->stack = stack;
	xf->arg = arg;
	_xil_invoke(&xf->frame, false);
	gsfree_assert (xf->frame.rc == XAX_SUCCESS);
	int tid = xf->out_tid;
	_xax_free_io_frame(&xf->frame);
	return tid;
*/
// Well, that's what you'd have done if you were really handled by a monitor
// process. But we're semicheating, and there's no easy way for the monitor
// to adjust my process. Time to call clone.

	PROTECT_GS_START();
	int flags = CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND|CLONE_THREAD|CLONE_SYSVSEM;

	// libc clone moves $esp down the stack some distance, so that it doesn't
	// end up where we asked for it (and hence where the caller's args
	// are stashed). To work around that, we use a trampoline.
	// But we can use clone's user 'arg' to avoid guessing at the stack.

	// clone thinks we're gonna return an int. Ha!
#if 0
	typedef int (clone_fn)(void *);
	rc = clone((clone_fn*) func, stack, flags, NULL);
#endif
	
	rc = _raw_clone(flags, stack, (void*) func);

	PROTECT_GS_END();
	return (rc==-1) ? false : true;
}

void xil_thread_exit(void)
{
	CALL_COUNT(zoog_thread_exit);
	syscall(__NR_exit);
}

int _internal_thread_get_id(void)
{
	return syscall(__NR_gettid);
}

#if 0
int xil_thread_get_id(void)
{
	PROTECT_GS_START();
	return _internal_thread_get_id();
	PROTECT_GS_END();
}
#endif

int kernel_set_thread_area(struct user_desc *ud)
{
	int entry_number;
	int _result;

	// not sure how this works: syscall returns two values?
	// how do the "m" args actually work!?

	asm volatile(
		"int $0x80\n\t"
		: "=a" (_result), "=m"(entry_number)
		: "0" (__NR_set_thread_area),
			"b"(ud),
			"m"(*ud)
		);
	return _result;
}

uint32_t get_gs(void)
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

void xil_x86_set_segments(uint32_t fs, uint32_t gs)
{
	CALL_COUNT(zoog_x86_set_segments);
	// can breakpoint here to see app-level calls, skipping over
	// internal PROTECT_GS macros.
	xil_x86_set_segments_internal(fs, gs);
}

void xil_x86_set_segments_internal(uint32_t fs, uint32_t gs)
{
/*
	XfX86setSegments *xf =
		(XfX86setSegments *) _xax_alloc_io_frame(sizeof(XfX86setSegments));
	xf->frame.opcode = XAX_OP_X86_SET_SEGMENTS;
	xf->fs = fs;
	xf->gs = gs;
	_xil_invoke(&xf->frame, false);
	gsfree_assert (xf->frame.rc == XAX_SUCCESS);
	_xax_free_io_frame(&xf->frame);
*/
	gsfree_assert(fs==0);

	int segment_selector = get_gs();

	struct user_desc ud;
	ud.entry_number = (segment_selector >> 3);
	ud.base_addr = gs;
	ud.limit = 0xfffff;
	ud.seg_32bit=1;
	ud.contents=0;
	ud.read_exec_only=0;
	ud.limit_in_pages=1;
	ud.seg_not_present=0;
	ud.useable=1;
	int rc = kernel_set_thread_area(&ud);
	gsfree_assert(rc==0);

	// Load the segment register to cause CPU to reload segment info
	// from GDT. (See note in glibc-2.7/nptl/sysdeps/i386/tls.h before the call
	// to TLS_SET_GS.) x86 sure stinks.
	// Yes, notice we're loading into gs the same thing we just read out of
	// it.
	__asm("movw %w0, %%gs" :: "q" (segment_selector));
}

void break_here_for_memtracker()
{
}

void xil_zutex_wait(ZutexWaitSpec *specs, uint32_t count)
{
	CALL_COUNT(zoog_zutex_wait);
	PROTECT_GS_START();
	_xil_zutex_wait_inner(specs, count);
	PROTECT_GS_END();
	break_here_for_memtracker();
}

void _xil_zutex_wait_inner(ZutexWaitSpec *specs, uint32_t count)
{
	if (count==0)
	{
		// Easiest. Wait. Ever.
		return;
	}

	PalState *pal = &g_pal_state;

	int msg_len = sizeof(XpReqZutexWaitPrepare)+sizeof(uint32_t*)*count;
	XpReqZutexWaitPrepare *req = (XpReqZutexWaitPrepare *) alloca(msg_len);
	INIT_VARIABLE_REQUEST(req, msg_len);

	req->thread_id = _internal_thread_get_id();
		// TODO it's not so clear we need a thread id in the
		// monitor's implementation, I just haven't gotten around to
		// removing it.
	req->count = count;

	uint32_t i;
	for (i=0; i<count; i++)
	{
		req->specs[i] = specs[i].zutex;
	}
	XpReplyZutexWait reply;
	_xax_port_rpc(pal, req, &reply, sizeof(reply));

	bool abort = false;
	for (i=0; i<count; i++)
	{
		if (specs[i].match_val != *(specs[i].zutex))
		{
			abort = true;
			break;
		}
	}

	char msg[200]; msg[0]='\0';
	if (chatty >= pal->ze->threshhold())
	{
		lite_strcpy(msg, "on guest zutexes ");
		for (i=0; i<count; i++)
		{
			char buf[40];
			cheesy_snprintf(buf, sizeof(buf), "Z0x%08x=%08x ", specs[i].zutex, specs[i].match_val);
			// Why do I write buffer overflows? Do I love C?
			lite_assert(lite_strlen(msg)+lite_strlen(buf)+1 <= sizeof(msg));	// sorry, debug msg too long.
			lite_strcat(msg, buf);
		}
	}
	bool slept;

	// Rather than re-construct req, we just poke the opcode.
	// (NB danger deep knowledge of matching request structures.)
	if (abort)
	{
		// won't block -- something changed after we locked in "kernel"
		// (monitor).  unlock and let user mode reinspect state.
		req->opcode = xpZutexWaitAbort;
		ZLC_CHATTY(pal->ze, "Thread %d aborts on %s\n",, req->thread_id, msg);
		slept = false;
	}
	else
	{
		// this will block, generally.
		req->opcode = xpZutexWaitCommit;
		ZLC_CHATTY(pal->ze, "Thread %d sleeps on %s\n",, req->thread_id, msg);
		slept = true;
	}

	_xax_port_rpc(pal, req, &reply, sizeof(reply));

	if (slept)
	{
		ZLC_CHATTY(pal->ze, "Thread %d wakes on %s\n",, req->thread_id, msg);
	}
}

int xil_zutex_wake(
	uint32_t *zutex,
	uint32_t match_val, 
	Zutex_count n_wake,
	Zutex_count n_requeue,
	uint32_t *requeue_zutex,
	uint32_t requeue_match_val)
{
	CALL_COUNT(zoog_zutex_wake);
	int result;
	PROTECT_GS_START();
	result = _xil_zutex_wake_inner(zutex, match_val, n_wake, n_requeue, requeue_zutex, requeue_match_val);
	PROTECT_GS_END();
	return result;
}

int _xil_zutex_wake_inner(
	uint32_t *zutex,
	uint32_t match_val, 
	Zutex_count n_wake,
	Zutex_count n_requeue,
	uint32_t *requeue_zutex,
	uint32_t requeue_match_val)
{
	PalState *pal = &g_pal_state;
	int result;

	DECLARE_REQUEST(ZutexWake, req);

	req.thread_id = _internal_thread_get_id();
	req.zutex = zutex;
	req.match_val = match_val;
	req.n_wake = n_wake;
	req.n_requeue = n_requeue;
	req.requeue_zutex = requeue_zutex;
	req.requeue_match_val = requeue_match_val;

//	tinyprintf("_xil_zutex_wake_inner", (uint32_t) zutex);

	XpReplyZutexWake reply;
	_xax_port_rpc(pal, &req, &reply, sizeof(reply));

	if (!reply.success)
	{
		result = -1;
	}
	else
	{
		// TODO um, we should return something here, and we're not.
		result = 0;
	}

	return result;
}

ZoogHostAlarms *xil_get_alarms()
{
	CALL_COUNT(zoog_get_alarms);
	PalState *pal = &g_pal_state;
	return pal->host_alarms.get_guest_addr_all();
}

void xil_get_time(uint64_t *out_time)
{
	CALL_COUNT(zoog_get_time);
	PalState *pal = &g_pal_state;
	*out_time = pal->clock_alarm_thread->get_time();
}

void xil_set_clock_alarm(
	uint64_t time)
{
	CALL_COUNT(zoog_set_clock_alarm);
	PalState *pal = &g_pal_state;
	pal->clock_alarm_thread->reset_alarm(time);
}

void xil_exit(void)
{
	CALL_COUNT(zoog_exit);
	PROTECT_GS_START();
	_gsfree_syscall(__NR_exit_group);
	gsfree_assert(0);
	PROTECT_GS_END();
}

void xil_launch_application(
	SignedBinary *signed_binary)
{
	CALL_COUNT(zoog_launch_application);
	PalState *pal = &g_pal_state;

	uint32_t message_len = sizeof(SignedBinary)
		+ Z_NTOHG(signed_binary->cert_len)
		+ Z_NTOHG(signed_binary->binary_len);
	ZoogNetBuffer *znb = xil_alloc_net_buffer(message_len);
	memcpy(ZNB_DATA(znb), signed_binary, message_len);

	XaxPortBuffer *xpb = _xpb_from_xnb(znb);

	PROTECT_GS_START();

	DECLARE_REQUEST(Launch, req);
	req.open_buffer_handle = xpb->buffer_handle;

	XpReplyLaunch reply;
	_xax_port_rpc(pal, &req, &reply, sizeof(reply));
	PROTECT_GS_END();
}

// Ask the TCB to endorse a public key as living on _this_ machine
void xil_endorse_me(ZPubKey *key, uint32_t cert_buffer_len, uint8_t* cert_buffer)
{
	CALL_COUNT(zoog_endorse_me);
	lite_assert(key);
	lite_assert(cert_buffer);

	PalState *pal = &g_pal_state;
	PROTECT_GS_START();

	// Allocate space for the request and the return (both variable)
	int msg_len = sizeof(XpReqEndorseMe)+key->size();
	XpReqEndorseMe *req = (XpReqEndorseMe*) alloca(msg_len);
	int reply_msg_len = sizeof(XpReplyEndorseMe)+ZCert::MAX_CERT_SIZE;
	XpReplyEndorseMe *reply = (XpReplyEndorseMe*) alloca(reply_msg_len);

	req->message_len = msg_len;
	req->opcode = xpEndorseMe;
	req->process_id = pal->process_id;

	req->app_key_bytes_len = key->size();
	key->serialize(req->app_key_bytes);

	_xax_port_rpc(pal, req, reply, reply_msg_len);

	lite_assert(reply->app_cert_bytes_len <= cert_buffer_len);
	lite_memcpy(cert_buffer, reply->app_cert_bytes, reply->app_cert_bytes_len);
	
	PROTECT_GS_END();
}

// Ask the TCB for an app-specific secret
uint32_t xil_get_app_secret(uint32_t num_bytes_requested, uint8_t* buffer)
{
	CALL_COUNT(zoog_get_app_secret);
	XpReplyGetAppSecret *reply = NULL;
	PalState *pal = &g_pal_state;

	lite_assert(buffer);

	PROTECT_GS_START();
	DECLARE_REQUEST(GetAppSecret, req);

	req.num_bytes_requested = num_bytes_requested;

	// Allocate space for the return, since it's variable-sized
	// (response will always be <= num_bytes_requested)
	int reply_msg_len = sizeof(XpReplyGetAppSecret)+num_bytes_requested;
	reply = (XpReplyGetAppSecret*) alloca(reply_msg_len);

	_xax_port_rpc(pal, &req, reply, reply_msg_len);

	lite_memcpy(buffer, reply->secret_bytes, reply->num_bytes_returned);
	
	PROTECT_GS_END();
	
	return reply->num_bytes_returned;
}

// Ask the TCB for some randomness
uint32_t xil_get_random(uint32_t num_bytes_requested, uint8_t* buffer)
{
	CALL_COUNT(zoog_get_random);
	// TODO: Instantiate our own PRNG with randomness from monitor
	//       to reduce # of syscalls
	XpReplyGetRandom *reply = NULL;
	PalState *pal = &g_pal_state;

	lite_assert(buffer);

	PROTECT_GS_START();
	DECLARE_REQUEST(GetRandom, req);

	req.num_bytes_requested = num_bytes_requested;

	// Allocate space for the return, since it's variable-sized
	// (response will always be <= num_bytes_requested)
	int reply_msg_len = sizeof(XpReplyGetRandom)+num_bytes_requested;
	reply = (XpReplyGetRandom*) alloca(reply_msg_len);

	_xax_port_rpc(pal, &req, reply, reply_msg_len);

	lite_memcpy(buffer, reply->random_bytes, reply->num_bytes_returned);
	
	PROTECT_GS_END();
	
	return reply->num_bytes_returned;
}


void _xil_connect(ZPubKey *pub_key)
{
	// Send a signal to establish process state at the monitor.

	PalState *pal = &g_pal_state;

	PROTECT_GS_START();

	int pub_key_size = (pub_key==NULL) ? 0 : pub_key->size();
	int msg_len = sizeof(XpReqConnect) + pub_key_size;
	XpReqConnect *req = (XpReqConnect *) alloca(msg_len);
	INIT_VARIABLE_REQUEST(req, msg_len);
	req->pub_key_len = pub_key_size;
	if (pub_key!=NULL)
	{
		pub_key->serialize((uint8_t*) &req[1]);
	}
	XpReplyConnect reply;
	_xax_port_rpc(pal, req, &reply, sizeof(reply));
	PROTECT_GS_END();
}

void _xil_start_receive_net_buffer_alarm_thread()
{
	// needs to happen after _xil_connect has completed.
	PalState *pal = &g_pal_state;
	pal->monitor_alarm_thread = new MonitorAlarmThread(
		pal->ze, &pal->host_alarms);
}

void debug_logfile_append_impl(const char *logfile_name, const char *message)
{
	PROTECT_GS_START();
	_debug_logfile_append_gsinner(logfile_name, message);
	PROTECT_GS_END();
}

void _debug_logfile_append_gsinner(const char *logfile_name, const char *message)
{
	PalState *pal = &g_pal_state;

	char buf[80];
	strcpy(buf, "debug_");
	strcat(buf, logfile_name);
	int dfd = _gsfree_syscall(__NR_open, buf, O_WRONLY|O_CREAT|O_APPEND, 0700);

	if (strcmp(logfile_name, "stderr")==0)
	{
		char time_prefix[80];
		strcpy(time_prefix, "[");
		_gsfree_itoa(&time_prefix[1], (int) (time(NULL) - pal->start_time));
		strcat(time_prefix, "] ");
		_gsfree_syscall(__NR_write, dfd, time_prefix, strlen(time_prefix));
		_gsfree_syscall(__NR_write, 2, message, strlen(message));
	}

	_gsfree_syscall(__NR_write, dfd, message, strlen(message));
	_gsfree_syscall(__NR_close, dfd);
}

void *debug_alloc_protected_memory(size_t length)
{
	void *result;
	PROTECT_GS_START();
	int rounded_length = (length+0xfff) & ~0xfff;
	int protected_length = rounded_length + 0x2000;
	uint8_t *protected_mem;
	protected_mem = (uint8_t *) mmap(NULL, protected_length,
		PROT_READ|PROT_WRITE|PROT_EXEC,
		MAP_PRIVATE|MAP_ANONYMOUS,
		-1,
		0);
	gsfree_assert(protected_mem !=MAP_FAILED);	// would explode touching gs anyway.
	int rc;
	rc = mprotect(protected_mem, 0x1000, 0);
	gsfree_assert(rc==0);
	rc = mprotect(protected_mem+protected_length-0x1000, 0x1000, 0);
	gsfree_assert(rc==0);
	result = protected_mem + 0x1000;
	PROTECT_GS_END();
	return result;
}

void debug_announce_thread(void)
{
	const char *msg = "debug_announce_thread\n";
	 _gsfree_syscall(__NR_write, 2, msg, strlen(msg));
	asm("int $3");
//	msg = "...survived\n";
//	 _gsfree_syscall(__NR_write, 2, msg, strlen(msg));
}

void debug_new_profiler_epoch(const char *name_of_last_epoch)
{
	// printf("End of epoch %s\n", name_of_last_epoch);
	char buf[200];
	cheesy_snprintf(buf, sizeof(buf), "End of epoch %s; handler counter %d\n", name_of_last_epoch, profiler.handler_called);
	safewrite_str(buf);
	lite_strncpy(profiler.epoch_name, name_of_last_epoch, sizeof(profiler.epoch_name));
	profiler.dump_flag = true;
	profiler.reset_flag = true;
}

ViewportID debug_create_toplevel_window()
{
	ViewportID result;
	PalState *pal = &g_pal_state;

	PROTECT_GS_START();

	DECLARE_REQUEST(DebugCreateToplevelWindow, req);
	XpReplyDebugCreateToplevelWindow reply;
	_xax_port_rpc(pal, &req, &reply, sizeof(reply));
	result = reply.tenant_id;
	PROTECT_GS_END();
	return result;
}
 

void debug_trace_touched_pages(void* addr, uint32_t size) {
	//printf("Protecting address %04x with size %d (%04x)\n", (uint32_t)addr, size, size);
	if (0 != _gsfree_syscall(__NR_mprotect, addr, size, PROT_NONE)) {
		perror("mprotect failed");
		gsfree_assert(false);
	}
}

int debug_cpu_times(struct tms *buf)
{
	return _gsfree_syscall(__NR_times, buf);
}

void *xil_lookup_extension(const char *name)
{
	CALL_COUNT(zoog_lookup_extension);
	PalState *pal = &g_pal_state;
	if (strcmp(name, "malloc_trace_funcs")==0)
	{
		return pal->mtfp;
	}
	else if (strcmp(name, DEBUG_LOGFILE_APPEND_NAME)==0)
	{
		return (void*) &debug_logfile_append_impl;
	}
	else if (strcmp(name, DEBUG_ALLOC_PROTECTED_MEMORY_NAME)==0)
	{
		return (void*) &debug_alloc_protected_memory;
	}
	else if (strcmp(name, DEBUG_ANNOUNCE_THREAD_NAME)==0)
	{
		return (void*) &debug_announce_thread;
	}
	else if (strcmp(name, DEBUG_NEW_PROFILER_EPOCH_NAME)==0)
	{
		return (void*) &debug_new_profiler_epoch;
	}
	else if (strcmp(name, DEBUG_CREATE_TOPLEVEL_WINDOW_NAME)==0)
	{
		return (void*) &debug_create_toplevel_window;
	}
	else if (strcmp(name, DEBUG_TRACE_TOUCHED_PAGES)==0)
	{
		return (void*) &debug_trace_touched_pages;
	}
	else if (strcmp(name, DEBUG_CPU_TIMES)==0)
	{
		return (void*) &debug_cpu_times;
	}

	return NULL;
}

void trace_alloc(TraceLabel label, void *addr, uint32_t size)
{
	PROTECT_GS_START();
	PalState *pal = &g_pal_state;
	gsfree_assert(label < tl_COUNT);
	mt_trace_alloc(pal->mt[label], (uint32_t) addr, size);
	PROTECT_GS_END();
}

void trace_free(TraceLabel label, void *addr)
{
	PROTECT_GS_START();
	PalState *pal = &g_pal_state;
	gsfree_assert(label < tl_COUNT);
	mt_trace_free(pal->mt[label], (uint32_t) addr);
	PROTECT_GS_END();
}

MallocFactory *mfarena_init(MFArena *mfa)
{
	concrete_mmap_pal_init(&mfa->cmp);
	cheesy_malloc_init_arena(&mfa->cma, &mfa->cmp.ami);
	cmf_init(&mfa->cmf, &mfa->cma);
	return &mfa->cmf.mf;
}

void xil_unimpl()
{
	lite_assert(false);
}

void xil_sublet_viewport(
	ViewportID tenant_viewport,
	ZRectangle *rectangle,
	ViewportID *out_landlord_viewport,
	Deed *out_deed)
{
	CALL_COUNT(zoog_sublet_viewport);
	PalState *pal = &g_pal_state;

	PROTECT_GS_START();

	DECLARE_REQUEST(SubletViewport, req);
	req.tenant_viewport = tenant_viewport;
	req.rectangle = *rectangle;

	XpReplySubletViewport reply;
	_xax_port_rpc(pal, &req, &reply, sizeof(reply));
	*out_landlord_viewport = reply.out_landlord_viewport;
	*out_deed = reply.out_deed;
	PROTECT_GS_END();
}

void xil_repossess_viewport(
	ViewportID landlord_viewport)
{
	CALL_COUNT(zoog_repossess_viewport);
	PalState *pal = &g_pal_state;

	PROTECT_GS_START();

	DECLARE_REQUEST(RepossessViewport, req);
	req.landlord_viewport = landlord_viewport;

	XpReplyRepossessViewport reply;
	_xax_port_rpc(pal, &req, &reply, sizeof(reply));
	PROTECT_GS_END();
}

void xil_get_deed_key(
	ViewportID landlord_viewport,
	DeedKey *out_deed_key)
{
	CALL_COUNT(zoog_get_deed_key);
	PalState *pal = &g_pal_state;
	PROTECT_GS_START();
	DECLARE_REQUEST(GetDeedKey, req);
	req.landlord_viewport = landlord_viewport;
	XpReplyGetDeedKey reply;
	_xax_port_rpc(pal, &req, &reply, sizeof(reply));
	*out_deed_key = reply.out_deed_key;
	PROTECT_GS_END();
}

void xil_accept_viewport(
	Deed *deed,
	ViewportID *out_tenant_viewport,
	DeedKey *out_deed_key)
{
	CALL_COUNT(zoog_accept_viewport);
	PalState *pal = &g_pal_state;
	PROTECT_GS_START();
	DECLARE_REQUEST(AcceptViewport, req);
	req.deed = *deed;
	XpReplyAcceptViewport reply;
	_xax_port_rpc(pal, &req, &reply, sizeof(reply));
	*out_tenant_viewport = reply.tenant_viewport;
	*out_deed_key = reply.deed_key;
	PROTECT_GS_END();
}

void xil_transfer_viewport(
	ViewportID tenant_viewport,
	Deed *out_deed)
{
	CALL_COUNT(zoog_transfer_viewport);
	PalState *pal = &g_pal_state;
	PROTECT_GS_START();
	DECLARE_REQUEST(TransferViewport, req);
	req.tenant_viewport = tenant_viewport;
	XpReplyTransferViewport reply;
	_xax_port_rpc(pal, &req, &reply, sizeof(reply));
	*out_deed = reply.deed;
	PROTECT_GS_END();
}


#if 0
ViewportID xil_delegate_viewport(ZCanvasID canvas_id, VendorIdentity *vendor)
{
	PalState *pal = &g_pal_state;

	ViewportID result;
	PROTECT_GS_START();

	int msg_len = sizeof(XpReqDelegateViewport)+vendor->size();
	XpReqDelegateViewport *req = (XpReqDelegateViewport *) alloca(msg_len);
	INIT_VARIABLE_REQUEST(req, msg_len);
	req->canvas_id = canvas_id;
	req->app_pub_key_bytes_len = vendor->size();
	vendor->serialize((uint8_t*) &req[1]);
	XpReplyDelegateViewport reply;
	_xax_port_rpc(pal, req, &reply, sizeof(reply));
	result = reply.viewport_id;
	PROTECT_GS_END();

	return result;
}

void xil_update_viewport(
	ViewportID viewport_id, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
	PalState *pal = &g_pal_state;

	PROTECT_GS_START();
	DECLARE_REQUEST(UpdateViewport, req);
	req.viewport_id = viewport_id;
	req.x = x;
	req.y = y;
	req.width = w;
	req.height = h;
	XpReplyUpdateViewport reply;
	_xax_port_rpc(pal, &req, &reply, sizeof(reply));
	PROTECT_GS_END();
}

void xil_close_viewport(ViewportID viewport_id)
{
	xil_unimpl();
}
#endif

// Verify the legitimacy of the label associated with this principal
// using the supplied certificate chain    
bool xil_verify_label(ZCertChain* chain)
{
	CALL_COUNT(zoog_verify_label);
	XpReplyVerifyLabel reply; 
	PalState *pal = &g_pal_state;

	lite_assert(chain);

	PROTECT_GS_START();

	// Allocate space for the request 
	// (it's variable len so we allocate it manually)
	int msg_len = sizeof(XpReqVerifyLabel)+chain->size();
	XpReqVerifyLabel *req = (XpReqVerifyLabel*) alloca(msg_len);

	req->message_len = msg_len;
	req->opcode = xpVerifyLabel;
	req->process_id = pal->process_id;

	chain->serialize(req->cert_chain_bytes);

	_xax_port_rpc(pal, req, &reply, sizeof(reply));

	PROTECT_GS_END();

	return reply.success;
}

void xil_map_canvas(
	ViewportID viewport_id, PixelFormat *known_formats, int num_formats,
	ZCanvas *out_canvas)
{
	CALL_COUNT(zoog_map_canvas);
	PalState *pal = &g_pal_state;
	PROTECT_GS_START();
	DECLARE_REQUEST(MapCanvas, req);
	req.thread_id = _internal_thread_get_id();
	req.viewport_id = viewport_id;
	gsfree_assert(num_formats*sizeof(PixelFormat) <= sizeof(req.known_formats));
	memcpy(req.known_formats, known_formats, num_formats*sizeof(PixelFormat));
	req.num_formats = num_formats;
	XpReplyMapCanvas reply;
	_xax_port_rpc(pal, &req, &reply, sizeof(reply));
	decode_canvas_reply(pal, &reply, out_canvas);
	PROTECT_GS_END();
}

void xil_unmap_canvas(ZCanvasID canvas_id)
{
	CALL_COUNT(zoog_unmap_canvas);
	PalState *pal = &g_pal_state;
	PROTECT_GS_START();
	DECLARE_REQUEST(UnmapCanvas, req);
	req.canvas_id = canvas_id;
	XpReplyUnmapCanvas reply;
	_xax_port_rpc(pal, &req, &reply, sizeof(reply));
	PROTECT_GS_END();
}

void xil_update_canvas(ZCanvasID canvas_id, ZRectangle *rectangle)
{
	CALL_COUNT(zoog_update_canvas);
	PalState *pal = &g_pal_state;
	PROTECT_GS_START();
	DECLARE_REQUEST(UpdateCanvas, req);
	req.canvas_id = canvas_id;
	req.rectangle = *rectangle;
	XpReplyUpdateCanvas reply;
	_xax_port_rpc(pal, &req, &reply, sizeof(reply));
	PROTECT_GS_END();
}

void xil_receive_ui_event(ZoogUIEvent *out_event)
{
	CALL_COUNT(zoog_receive_ui_event);
	PalState *pal = &g_pal_state;
	PROTECT_GS_START();
	DECLARE_REQUEST(ReceiveUIEvent, req);
	XpReplyReceiveUIEvent reply;
	_xax_port_rpc(pal, &req, &reply, sizeof(reply));
	*out_event = reply.event;
	PROTECT_GS_END();
}

#if TRACE_PAGES_TOUCHED 
#define PAGE_SIZE 4096
#define LOG_PAGE_SIZE 12

static void signal_handler(int signal, siginfo_t *info, void *unused) {
	if (info != NULL) {
		//printf("Got signal #%d with addr of info at %04x and fault addr %04x\n", signal, (uint32_t)info, (uint32_t)info->si_addr);
		uint8_t* alignedAddr = (uint8_t*)((((unsigned int)(info->si_addr))>>LOG_PAGE_SIZE) * PAGE_SIZE);
		
		_gsfree_syscall(__NR_mprotect, alignedAddr, PAGE_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC);
		//printf("Mapped in a page starting at %0x4\n", (uint32_t)alignedAddr);
		char buf[200];
		cheesy_snprintf(buf, sizeof(buf), "Mapped in a page starting at %x\n", (uint32_t)alignedAddr);
		_gsfree_syscall(__NR_write, 2, buf, lite_strlen(buf));
	} else {
		gsfree_assert(false);
		printf("Got signal #%d with NULL info!!!\n", signal);
	}
}

#endif // TRACE_PAGES_TOUCHED 

PalState *pal_state_init(void)
{
	PalState *pal = &g_pal_state;

	pal->process_id = getpid();
	xax_pipe_pool_init(&pal->pipe_pool);

	// make a cheesy allocator we can use inside pal
	// since cheesylock isn't reentrant, the profiler needs
	// a private arena
	pal->mf = mfarena_init(&pal->main_mf_arena);
	pal->profiler_mf = mfarena_init(&pal->profiler_mf_arena);
	malloc_factory_operator_new_init(pal->mf);
	ambient_malloc_init(pal->mf);

	TunIDAllocator tunid_allocator;
	pal->tunid = tunid_allocator.get_tunid();

	pal->xpb_table = new XPBTable(pal->mf);

	memset(&pal->host_alarms, 0, sizeof(pal->host_alarms));

	pal->ze = new ZLCEmitGsfree(terse);
	pal->clock_alarm_thread = new ClockAlarmThread(pal->ze, &pal->host_alarms);

#if TRACE_PAGES_TOUCHED 
	int page_size = getpagesize();
	gsfree_assert(page_size == PAGE_SIZE);
	// Register a handler to catch SIGFAULTs 
	// caused by our mprotect memory measurements
	struct sigaction action;
	sigemptyset(&action.sa_mask);
	action.sa_flags = SA_SIGINFO;
	action.sa_sigaction = signal_handler;
	if (sigaction(SIGSEGV, &action, NULL) == -1) {
		perror("Unable to register SIGSEGV handler");
		gsfree_assert(false);
	}
#endif // TRACE_PAGES_TOUCHED 

	int mi;
	for (mi=0; mi<tl_COUNT; mi++)
	{
		pal->mt[mi] = mt_init(pal->mf);
	}
	pal->mtf.tr_alloc = &trace_alloc;
	pal->mtf.tr_free = &trace_free;
	pal->mtfp = &pal->mtf;

	pal->xil_dispatch_table.zoog_lookup_extension = xil_lookup_extension;
	pal->xil_dispatch_table.zoog_allocate_memory = xil_allocate_memory;
	pal->xil_dispatch_table.zoog_free_memory = xil_free_memory;
	pal->xil_dispatch_table.zoog_thread_create = xil_thread_create;
	pal->xil_dispatch_table.zoog_thread_exit = xil_thread_exit;
	pal->xil_dispatch_table.zoog_x86_set_segments = xil_x86_set_segments;
	pal->xil_dispatch_table.zoog_exit = xil_exit;
	pal->xil_dispatch_table.zoog_launch_application = xil_launch_application;
	pal->xil_dispatch_table.zoog_endorse_me = xil_endorse_me;
	pal->xil_dispatch_table.zoog_get_app_secret = xil_get_app_secret;
	pal->xil_dispatch_table.zoog_get_random = xil_get_random;

	pal->xil_dispatch_table.zoog_zutex_wait = xil_zutex_wait;
	pal->xil_dispatch_table.zoog_zutex_wake = xil_zutex_wake;
	pal->xil_dispatch_table.zoog_get_alarms = xil_get_alarms;

	pal->xil_dispatch_table.zoog_get_ifconfig = xil_get_ifconfig;
	pal->xil_dispatch_table.zoog_alloc_net_buffer = xil_alloc_net_buffer;
	pal->xil_dispatch_table.zoog_send_net_buffer = xil_send_net_buffer;
	pal->xil_dispatch_table.zoog_receive_net_buffer = xil_receive_net_buffer;
	pal->xil_dispatch_table.zoog_free_net_buffer = xil_free_net_buffer;

	pal->xil_dispatch_table.zoog_get_time = xil_get_time;
	pal->xil_dispatch_table.zoog_set_clock_alarm = xil_set_clock_alarm;

	pal->xil_dispatch_table.zoog_sublet_viewport	= xil_sublet_viewport;
	pal->xil_dispatch_table.zoog_repossess_viewport	= xil_repossess_viewport;
	pal->xil_dispatch_table.zoog_accept_viewport	= xil_accept_viewport;
	pal->xil_dispatch_table.zoog_get_deed_key		= xil_get_deed_key;
	pal->xil_dispatch_table.zoog_transfer_viewport	= xil_transfer_viewport;
	pal->xil_dispatch_table.zoog_verify_label		= xil_verify_label;
	pal->xil_dispatch_table.zoog_map_canvas			= xil_map_canvas;
	pal->xil_dispatch_table.zoog_unmap_canvas		= xil_unmap_canvas;
	pal->xil_dispatch_table.zoog_update_canvas		= xil_update_canvas;
	pal->xil_dispatch_table.zoog_receive_ui_event	= xil_receive_ui_event;

	pal->sf = new SyncFactory_GsfreeFutex();

	profiler_init(&pal->profiler, pal->profiler_mf);
	pal->start_time = time(NULL);

#if CALL_COUNT_ENABLED
	{
		pal->call_counts = new CallCounts();
		int stack_size = 8<<12;
		pal_thread_create(CallCounts::dump_wait_thread, pal->call_counts, stack_size);
	}
#endif // CALL_COUNT_ENABLED

	return pal;
}
	
ZoogDispatchTable_v1 *xil_get_dispatch_table()
{
	return &g_pal_state.xil_dispatch_table;
}

void dump_traces(TraceLabel label, int level)
{
	PalState *pal = &g_pal_state;
	PROTECT_GS_START();

	mt_dump_traces(pal->mt[label], level);

	PROTECT_GS_END();
}

