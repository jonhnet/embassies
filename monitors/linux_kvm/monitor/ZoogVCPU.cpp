#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/kvm.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdint.h>
#include <signal.h>
#include <syscall.h>
#include <malloc.h>

#include "ZoogVM.h"
#include "ZoogVCPU.h"
#include "VCPUPool.h"
#include "MemSlot.h"
#include "LiteLib.h"
#include "safety_check.h"
#include "CanvasAcceptor.h"
//#include "ViewportDelegator.h"
#include "CryptoException.h"
#include "KeyDerivationKey.h"
#include "math_util.h"
#include "KvmNetBufferContainer.h"

/*
 * NB: Some methods in this file follow a safety-check design pattern
 * of copying the fixed part of the xa_* data structure from guest
 * memory (where it could be changed in a TOCTTOU attack) to
 * host memory, and then checking the parameters where we know
 * they can't be changed out from under us.
 *
 * We may then find ourselves wanting to access a variable-length
 * region from guest's call page. Sometimes it's okay to do this directly,
 * because once we've done bounds checks, our access to that region
 * doesn't affect our control flow, so a data race couldn't have
 * any safety effect.
 *
 * Then we write results back into the guest through the xa_guest
 * pointer.
 */

ZoogVCPU::ZoogVCPU(ZoogVM *vm, uint32_t guest_entry_point)
{
	uint32_t stack_size = 0x1000;
	uint32_t stack_bottom_guest =
		vm->allocate_guest_memory(stack_size, "vcpu_stack")->get_guest_addr();
	uint32_t stack_top_guest = stack_bottom_guest + stack_size;

	_init(vm, guest_entry_point, stack_top_guest);
}

ZoogVCPU::ZoogVCPU(ZoogVM *vm, uint32_t guest_entry_point, uint32_t stack_top_guest)
{
	_init(vm, guest_entry_point, stack_top_guest);
}

void ZoogVCPU::_init(ZoogVM *vm, uint32_t guest_entry_point, uint32_t stack_top_guest)
{
	this->vm = vm;
	this->zid = vm->allocate_zid();
	this->guest_entry_point = guest_entry_point;
	this->stack_top_guest = stack_top_guest;
	this->thread_condemned = false;
	this->zutex_waiter = new ZutexWaiter(vm->get_malloc_factory(), vm->get_sync_factory(), this);

	init_pause_controls();

	vcpu_allocation = NULL;
	vm->record_vcpu(this);

	pthread_create(&pt, NULL, zvcpu_thread, this);
}

ZoogVCPU::~ZoogVCPU()
{
	vm->remove_vcpu(this);
	free_pause_controls();
	delete(zutex_waiter);
	vm->free_guest_memory(gdt_page);
	yield_kvm_vcpu();
}

void *ZoogVCPU::zvcpu_thread(void *obj)
{
	((ZoogVCPU*) obj)->run();
	return NULL;
}

void break_here()
{
	int x=7;
	x+=1;
}

void ZoogVCPU::_init_segment(kvm_segment *seg, uint32_t base, int type, int s, int selector)
{
	seg->base = base;
	seg->limit = 0xffffffff;
	seg->selector = selector;
	seg->type = type;
	seg->present = 1;
	seg->s = s;
	seg->g = 1;	// granularity: limit in 4kB units
	seg->db = 1;
}

void ZoogVCPU::gdt_set_gate(uint32_t idx, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity)
{
	gdt_entries[idx].base_low = (base & 0xffff);
	gdt_entries[idx].base_middle = (base >>16) & 0xff;
	gdt_entries[idx].base_high = (base >>24) & 0xff;
	gdt_entries[idx].limit_low = (limit & 0xffff);
	gdt_entries[idx].granularity =
		  ((limit >> 16) & 0x0f)
		| (granularity & 0xf0);
	gdt_entries[idx].access = access;
}

void ZoogVCPU::kablooey(bool wait_patiently)
{
	int rc;
	struct kvm_regs regs;
	rc = ioctl(vcpufd(), KVM_GET_REGS, &regs);
	assert(rc==0);
	struct kvm_sregs sregs;
	rc = ioctl(vcpufd(), KVM_GET_SREGS, &sregs);
	assert(rc==0);
	fprintf(stderr, "rip %08Lx\n", regs.rip);
	fprintf(stderr, "exit_reason %d (decimal) hw %16Lx\n",
		vcpu_allocation->get_kvm_run()->exit_reason,
		vcpu_allocation->get_kvm_run()->hw.hardware_exit_reason);

	// be sure pause doesn't wait on me.
	is_paused_flag = true;
	pthread_cond_signal(&has_paused_cond);

	if (wait_patiently)
	{
		while (1)
		{
			sleep(1);
		}
	}
	lite_assert(false);
}

void ZoogVCPU::sig_int_absorber(int signum)
{
	fprintf(stderr, "ZoogVCPU::sig_int_absorber()\n");
}

int core_signal = SIGUSR1;

void ZoogVCPU::run()
{
	int rc;

	struct sigaction sig_int_action;
	sig_int_action.sa_handler = sig_int_absorber;
	sigemptyset(&sig_int_action.sa_mask);
	sig_int_action.sa_flags = 0;
	rc = sigaction(SIGUSR1, &sig_int_action, NULL);
	assert(rc==0);

	struct kvm_regs regs;
	memset(&regs, 0, sizeof(regs));
	regs.rip = (uint64_t) guest_entry_point;
	regs.rsp = (uint64_t) stack_top_guest;
	regs.rflags = 0x2;

	struct kvm_sregs sregs;
	memset(&sregs, 0, sizeof(sregs));
	_init_segment(&sregs.cs, 0, 0xb, 1, gdt_index_to_selector(gdt_code_flat));
	_init_segment(&sregs.ds, 0, 3, 1, gdt_index_to_selector(gdt_data_flat));
	_init_segment(&sregs.es, 0, 3, 1, gdt_index_to_selector(gdt_data_flat));
	_init_segment(&sregs.fs, 0, 3, 1, gdt_index_to_selector(gdt_fs_user));
	_init_segment(&sregs.gs, 0, 3, 1, gdt_index_to_selector(gdt_gs_user));
	_init_segment(&sregs.ss, 0, 3, 1, gdt_index_to_selector(gdt_data_flat));
	_init_segment(&sregs.tr, 0, 0xb, 0, 0);

	gdt_page = vm->allocate_guest_memory(PAGE_SIZE, "vcpu_gdt_page");
	uint32_t gdt_guest_addr = (uint32_t) gdt_page->get_guest_addr();

	uint32_t gdt_size = sizeof(struct gdt_table_entry)*gdt_count;
	// We'll pack the call_page into the same page, just after the gdt.
	uint32_t call_page_guest_address = gdt_guest_addr + gdt_size;
	call_page =
		(ZoogKvmCallPage *) (gdt_page->get_host_addr() + gdt_size);
	call_page->guest_address = (uint32_t*) call_page_guest_address;
	call_page->dbg_magic = ZOOG_KVM_CALL_PAGE_MAGIC;
	call_page_capacity = gdt_page->get_size() - (gdt_size + sizeof(*call_page));
	call_page->capacity = call_page_capacity;

	this->gdt_entries = (struct gdt_table_entry*) gdt_page->get_host_addr();
	gdt_set_gate(gdt_null, 0, 0, 0, 0);
	gdt_set_gate(gdt_code_flat, 0, 0xffffffff, gdt_access_code, gdt_granularity);
	gdt_set_gate(gdt_data_flat, 0, 0xffffffff, gdt_access_data, gdt_granularity);
	// user segments initially behave like flat segments, until user asks
	// for something. ("undefined" would be even better -- TODO maybe
	// we should have a limit 0 to detect misbehaving software that
	// uses fs/gs before configuring them.)
	gdt_set_gate(gdt_fs_user, 0, 0xffffffff, gdt_access_data, gdt_granularity);
	gdt_set_gate(gdt_gs_user, 0, 0xffffffff, gdt_access_data, gdt_granularity);
	gdt_set_gate(gdt_gs_pal, call_page_guest_address, 0xffffffff, gdt_access_data, gdt_granularity);

	sregs.gdt.base = gdt_guest_addr;
	sregs.gdt.limit = gdt_size;
	// Configuring LDT, even thought I'm pretty sure we aren't going to
	// ever touch it.
	sregs.ldt.base = gdt_guest_addr;
	sregs.ldt.limit = gdt_size;
	sregs.ldt.present = 1;
	sregs.ldt.type = 2;

	// TODO don't need one of these per thread...
	sregs.idt.base = vm->get_idt_guest_addr();
	sregs.idt.limit = 0x0fff;

	sregs.cr0 = 0;
	sregs.cr0 |= (1<<30); // cache disable
	sregs.cr0 |= (1<<29); // not write-through
	sregs.cr0 |= (1<<4);  // extension type
	sregs.cr0 |= (1<<1);  // monitor coprocessor
	sregs.cr0 |= (1<<0);  // protection enable

	sregs.cr4 = 0;
	sregs.cr4 |= (1<<9); // Set the OSFXSR bit		(MMX/SSE support)
	sregs.cr4 |= (1<<10); // Set the OSXMMEXCPT bit (MMX/SSE support)
	sregs.cr4 |= (1<<18); // Enable use of XSETBV/XGETBC (AVX support)

	my_thread_id = syscall(__NR_gettid); //gettid();

	// Transfer the locals we set up into the snap regs, where they'll
	// be installed when the VCPU is claimed.
	snap_regs = regs;
	snap_sregs = sregs;

	//vm->dbg_dump_memory_map();
	service_loop();
}

void ZoogVCPU::init_pause_controls()
{
	my_thread_id = 0;
	pthread_mutex_init(&pause_mutex, NULL);
	pthread_cond_init(&done_pause_cond, NULL);
	pthread_mutex_init(&has_paused_mutex, NULL);
	pthread_cond_init(&has_paused_cond, NULL);
	pause_control_flag = false;
	is_paused_flag = false;
}

void ZoogVCPU::free_pause_controls()
{
	pthread_mutex_destroy(&pause_mutex);
	pthread_cond_destroy(&done_pause_cond);
	pthread_mutex_destroy(&has_paused_mutex);
	pthread_cond_destroy(&has_paused_cond);
}

void ZoogVCPU::pause()
{
	pause_control_flag = true;
	lite_assert(my_thread_id!=0);	// lazy me didn't synchronize

	// NB it may be the case that 'is_pause_flag' is already true,
	// because the thread is sitting in a (perhaps long-duration) handler
	// routine. In that case, we don't bother sendig a kill; our
	// setting pause_control_flag is enough to be certain that the
	// thread won't stumble back into resuming the VCPU.
	pthread_mutex_lock(&has_paused_mutex);
	while (!is_paused_flag)
	{
		fprintf(stderr, "kill(%d)\n", my_thread_id);
		kill(my_thread_id, SIGUSR1);
		pthread_cond_wait(&has_paused_cond, &has_paused_mutex);
	}
	pthread_mutex_unlock(&has_paused_mutex);
}

void ZoogVCPU::resume()
{
	pause_control_flag = false;
	pthread_cond_signal(&done_pause_cond);
}

void ZoogVCPU::claim_kvm_vcpu()
{
	int rc;
	if (vcpu_allocation == NULL)
	{
		// fprintf(stderr, "ZoogVCPU claims VCPU\n");
		vcpu_allocation = vm->get_vcpu_pool()->allocate();
		lite_assert(vcpu_allocation != NULL);

		rc = ioctl(vcpufd(), KVM_SET_REGS, &snap_regs);
		assert(rc==0);
		// Not sure why we had these GETs here; I think it was early debugging.
		// rc = ioctl(vcpufd(), KVM_GET_REGS, &regs);
		// assert(rc==0);

		rc = ioctl(vcpufd(), KVM_SET_SREGS, &snap_sregs);
		assert(rc==0);
		//rc = ioctl(vcpufd(), KVM_GET_SREGS, &sregs);
		//assert(rc==0);
		// to see gdt in gdb:
		// x/32xb vm->memory+0x1000
		//break_here();
	}
}

void ZoogVCPU::yield_kvm_vcpu()
{
	int rc;

	// TODO if user code has useful state in its SSE/MMX/AVR registers
	// when it zutex_waits (or if we later extend this code to allow
	// preemption), then we'd need to learn to use the processor features
	// (or the kvm view of them)
	// to detect the has-been-accessed state of those registers and
	// save them out before we yield the VCPU.

	if (vcpu_allocation != NULL)
	{
		// fprintf(stderr, "ZoogVCPU yields VCPU\n");
		rc = ioctl(vcpufd(), KVM_GET_REGS, &snap_regs);
		assert(rc==0);
		rc = ioctl(vcpufd(), KVM_GET_SREGS, &snap_sregs);
		assert(rc==0);

		vm->get_vcpu_pool()->free(vcpu_allocation);
		vcpu_allocation=NULL;
	}
}

void ZoogVCPU::service_loop()
{
	int rc;
	while (!thread_condemned)
	{
		pthread_mutex_lock(&pause_mutex);
		while (pause_control_flag)
		{
			is_paused_flag = true;
			pthread_cond_signal(&has_paused_cond);
			pthread_cond_wait(&done_pause_cond, &pause_mutex);
		}
		pthread_mutex_unlock(&pause_mutex);

		is_paused_flag = false;

		claim_kvm_vcpu();

		rc = ioctl(vcpufd(), KVM_RUN, 0);
		if (rc!=0)
		{
			if (errno==EINTR)
			{
				// Huh. What signal was that!?
				continue;
			}
			else if (errno==EFAULT)
			{
				// wait, a fault in KVM_RUN? maybe this is an intel-VT
				// vs amd-VMX distinction?
				fprintf(stderr, "EFAULT: \?\?!\?\n");
				vm->request_coredump();
				kablooey(true);
			}
			else
			{
				kablooey(false);
			}
		}
		
		is_paused_flag = true;

		switch (vcpu_allocation->get_kvm_run()->exit_reason)
		{
		case KVM_EXIT_IO:
			fprintf(stderr, "KVM_EXIT_IO\n");
			break;
		case KVM_EXIT_HLT:
		{
			handle_zoogcall();
			break;
		}
		case KVM_EXIT_MMIO:
		{
			// huh -- it seemed to explode on zero-address memory.
			// which is odd; I don't think we ever unmapped the zero page.
			fprintf(stderr, "KVM_EXIT_MMIO: \?\?!\?\n");
			vm->request_coredump();
			kablooey(true);
		}
		case KVM_EXIT_SHUTDOWN:
		{
			fprintf(stderr, "EXIT_SHUTDOWN: triple-fault of my baaaaby's love (uh huh)\n");
			vm->request_coredump();
			kablooey(true); // sit around waiting for a dump
		}
		default:
		{
			fprintf(stderr, "Unexpected exit code\n");
			vm->request_coredump();
			kablooey(true);
		}
		}
	}
	delete this;
}

void ZoogVCPU::handle_zoogcall()
{
	XP_Opcode *opcode = (XP_Opcode *) call_page->call_struct;
	vm->get_call_counts()->tally((*opcode) - xp_allocate_memory);
	switch (*opcode)
	{
#define CASE(s)	case xp_##s: s((xa_##s *) call_page->call_struct); break;
	CASE(allocate_memory)
	CASE(free_memory)
	CASE(get_ifconfig)
	CASE(alloc_net_buffer)
	CASE(send_net_buffer)
	CASE(receive_net_buffer)
	CASE(free_net_buffer)
	CASE(thread_create)
	CASE(thread_exit)
	CASE(exit)
	CASE(x86_set_segments)
	CASE(zutex_wait)
	CASE(zutex_wake)
	CASE(get_time)
	CASE(set_clock_alarm)
	CASE(launch_application)
	CASE(endorse_me)
	CASE(get_app_secret)
	CASE(get_random)
	CASE(extn_debug_logfile_append)
	CASE(extn_debug_create_toplevel_window)
	CASE(internal_find_app_code)
	CASE(internal_map_alarms)
	CASE(sublet_viewport)
	CASE(repossess_viewport)
	CASE(get_deed_key)
	CASE(accept_viewport)
	CASE(transfer_viewport)
	CASE(verify_label)
	CASE(map_canvas)
	CASE(unmap_canvas)
	CASE(update_canvas)
	CASE(receive_ui_event)
	CASE(debug_set_idt_handler)
	default:
		unknown_zoogcall(*opcode);
		break;
	}
	*opcode = (XP_Opcode) (*opcode | 0x80000000);
		// invalidate opcode so next call doesn't look like one
		// unless it was correctly configured.
}

void ZoogVCPU::clean_string(char *dst_str, int capacity, const char *src_str)
{
	strncpy(dst_str, src_str, capacity-1);
	dst_str[capacity-1] = '\0';
	for (int i=0; i<capacity && dst_str[i]!='\0'; i++)
	{
		if (!isalnum(dst_str[i]))
		{
			dst_str[i] = '_';
		}
	}
}

void ZoogVCPU::extn_debug_logfile_append(xa_extn_debug_logfile_append *xa_guest)
{
	xa_extn_debug_logfile_append xa = *xa_guest;
	const int logfile_name_capacity = sizeof(xa.in_logfile_name)+1;
	SAFETY_CHECK(xa.in_msg_len <= call_page_capacity - sizeof(xa));

	char logfile_name[logfile_name_capacity];
	clean_string(logfile_name, logfile_name_capacity, xa.in_logfile_name);
	FILE *fp;
	char filename[logfile_name_capacity + 10];
	snprintf(filename, sizeof(filename), "debug_%s", logfile_name);
	fp = fopen(filename, "a");
	SAFETY_CHECK(fp!=NULL);
	fwrite(xa_guest->in_msg_bytes, xa.in_msg_len, 1, fp);
		// NB direct guest access after safety checks on copy; that's
		// okay here because the bytes themselves don't affect control
		// flow, so they can't pose a risk of TOCTTOU attack.
	fclose(fp);
#if DEBUG_VULNERABLY
	if (strcmp(logfile_name, "stderr")==0)
	{
		fwrite(xa_guest->in_msg_bytes, xa.in_msg_len, 1, stderr);
		fflush(stderr);
	}
#endif // DEBUG_VULNERABLY
fail:
	return;
}

void ZoogVCPU::extn_debug_create_toplevel_window(xa_extn_debug_create_toplevel_window *xa_guest)
{
	vm->get_coordinator()->send_extn_debug_create_toplevel_window(xa_guest);
}

void ZoogVCPU::unknown_zoogcall(XP_Opcode opcode)
{
	fprintf(stderr, "sadness: unimplemented zoogcall opcode %08x\n",
		opcode);
	int rc;

	struct kvm_sregs sregs;
	rc = ioctl(vcpufd(), KVM_GET_SREGS, &sregs);
	assert(rc==0);

	struct kvm_regs regs;
	rc = ioctl(vcpufd(), KVM_GET_REGS, &regs);
	assert(rc==0);

	fprintf(stderr, "At KVM_EXIT_HLT; rbx = %08x rip=%08x rsp=%08x\n",
		(int) regs.rbx, (int) regs.rip, (int) regs.rsp);

	break_here();
	SAFETY_CHECK(false);
fail:
	// TODO this would be an excellent time to ask the ZoogVM to kill this app!
	return;
}

void ZoogVCPU::allocate_memory(xa_allocate_memory *xa_guest)
{
	uint32_t req_size = xa_guest->in_length;
	MemSlot *mem_slot = vm->allocate_guest_memory(req_size, "app_allocated");
	if (mem_slot!=NULL)
	{
		xa_guest->out_pointer = (void*) mem_slot->get_guest_addr();
	}
	else
	{
		xa_guest->out_pointer = NULL;
		fprintf(stderr, "WARNING: failed memory allocation for %d bytes\n",
			req_size);
	}
}

void ZoogVCPU::unimpl(XP_Opcode opcode)
{
	fprintf(stderr, "Unimplemented: %08x\n", opcode);
	lite_assert(false);
}

void ZoogVCPU::free_memory(xa_free_memory *xa_guest)
{
	vm->free_guest_memory(
		(uint32_t) xa_guest->in_pointer, (uint32_t) xa_guest->in_length);
}

void ZoogVCPU::get_ifconfig(xa_get_ifconfig *xa_guest)
{
	int count;
	int space = sizeof(xa_guest->out_ifconfigs)/sizeof(xa_guest->out_ifconfigs[0]);
	XIPifconfig *ifconfigs = vm->get_coordinator()->get_ifconfigs(&count);
	int i;
	for (i=0; i<count && i<space; i++)
	{
		xa_guest->out_ifconfigs[i] = ifconfigs[i];
	}
	xa_guest->out_count = i;
}

void ZoogVCPU::alloc_net_buffer(xa_alloc_net_buffer *xa_guest)
{
	uint32_t request_size = xa_guest->in_payload_size;
	uint32_t required_size = request_size + sizeof(ZoogNetBuffer);
	NetBuffer *nb;

	if (request_size <= ZK_SMALL_MESSAGE_LIMIT)
	{
		nb = vm->get_net_buffer_table()->alloc_net_buffer(request_size);
	}
	else
	{
		LongMessageAllocation *lma =
			vm->get_coordinator()->send_alloc_long_message(required_size + KvmNetBufferContainer::PADDING);
		nb = vm->get_net_buffer_table()->install_net_buffer(lma, true);
	}
	if (nb==NULL)
	{
		xa_guest->out_zoog_net_buffer = NULL;
	}
	else
	{
		lite_assert(nb->get_host_znb_addr()->capacity >= request_size);
#if DBG_SEND_FAILURE
		fprintf(vm->dbg_send_log_fp, "alloc() = 0x%08x\n", nb->get_guest_znb_addr());
#endif
		xa_guest->out_zoog_net_buffer = (ZoogNetBuffer*) nb->get_guest_znb_addr();
	}
}

void ZoogVCPU::send_net_buffer(xa_send_net_buffer *xa_guest)
{
	xa_send_net_buffer xa = *xa_guest;
	uint32_t znb_guest = (uint32_t) xa.in_zoog_net_buffer;
	NetBufferTable *nbt = vm->get_net_buffer_table();
#if DBG_SEND_FAILURE
		fprintf(vm->dbg_send_log_fp, "send(0x%08x, %d)\n", znb_guest, xa.in_release);
#endif
	NetBuffer *nb = nbt->lookup_net_buffer(znb_guest);
	lite_assert(nb!=NULL);
	vm->get_coordinator()->send_packet(nb);
	if (xa.in_release)
	{
		_free_net_buffer(nb);
	}
}

void ZoogVCPU::_free_net_buffer(NetBuffer* nb)
{
	LongMessageAllocation* lma = nb->get_lma();
	if (lma!=NULL)
	{
		vm->get_coordinator()->send_free_long_message(lma);
	}
	vm->get_net_buffer_table()->free_net_buffer(nb);
}

void ZoogVCPU::receive_net_buffer(xa_receive_net_buffer *xa_guest)
{
	Packet *p = vm->get_coordinator()->dequeue_packet();
	NetBuffer *nb;
	if (p->get_lma()!=NULL)
	{
		nb = vm->get_net_buffer_table()->install_net_buffer(p->get_lma(), false);
		lite_assert(nb!=NULL);	// handle this case -- what, un-dequeue the packet?
	}
	else
	{
		uint32_t req_size = p->get_size();
		nb = vm->get_net_buffer_table()->alloc_net_buffer(req_size);
		lite_assert(nb!=NULL);	// handle this case -- what, un-dequeue the packet?
		if (nb==NULL)
		{
			// rats. Drop the packet?
			fprintf(stderr, "Couldn't make NB for %d-byte packet.\n", req_size);
			delete p;
			xa_guest->out_zoog_net_buffer = NULL;
		}
		else
		{
			lite_assert(nb->get_host_znb_addr()->capacity >= req_size);
			memcpy(ZNB_DATA(nb->get_host_znb_addr()), p->get_data(), req_size);
		}
	}
	xa_guest->out_zoog_net_buffer = (ZoogNetBuffer*) nb->get_guest_znb_addr();
}

void ZoogVCPU::free_net_buffer(xa_free_net_buffer *xa_guest)
{
	uint32_t znb_guest = (uint32_t) xa_guest->in_zoog_net_buffer;
	NetBufferTable *nbt = vm->get_net_buffer_table();
	NetBuffer *nb = nbt->lookup_net_buffer(znb_guest);
	_free_net_buffer(nb);
}

void ZoogVCPU::thread_create(xa_thread_create *xa_guest)
{
	new ZoogVCPU(vm, (uint32_t) xa_guest->in_func, (uint32_t) xa_guest->in_stack);
	xa_guest->out_result = true;	// confident, aren't we?
}

void ZoogVCPU::thread_exit(xa_thread_exit *xa_guest)
{
	thread_condemned = true;
}

void ZoogVCPU::exit(xa_exit *xa_guest)
{
	vm->destroy();
}

void ZoogVCPU::x86_set_segments(xa_x86_set_segments *xa_guest)
{
	// NB note that we change sregs "in place" (down in kvm's copy).
	// The snap_sregs aren't relevant, because we haven't yielded
	// the VCPU.
	lite_assert(vcpu_held());

	struct kvm_sregs sregs;
	int rc;
	rc = ioctl(vcpufd(), KVM_GET_SREGS, &sregs);
	lite_assert(rc==0);

	gdt_set_gate(gdt_fs_user, xa_guest->in_fs, 0xffffffff, gdt_access_data, gdt_granularity);
	_init_segment(&sregs.fs, xa_guest->in_fs, 3, 1, gdt_index_to_selector(gdt_fs_user));
	gdt_set_gate(gdt_gs_user, xa_guest->in_gs, 0xffffffff, gdt_access_data, gdt_granularity);
	_init_segment(&sregs.gs, xa_guest->in_gs, 3, 1, gdt_index_to_selector(gdt_gs_user));

	rc = ioctl(vcpufd(), KVM_SET_SREGS, &sregs);
	lite_assert(rc==0);
}

void ZoogVCPU::zutex_wait(xa_zutex_wait *xa_guest)
{
	xa_zutex_wait xa = *xa_guest;
	SAFETY_CHECK(xa.in_count*sizeof(ZutexWaitSpec)
		<= call_page_capacity - sizeof(xa));

	// Now we know it's safe to access xa.in_count (a host variable)
	// specs, so we can look right into them in the shared call page;
	// there's no further control-flow dependency.
	ZutexWaitSpec *specs;
	specs = xa_guest->in_specs;

	vm->get_zutex_table()->wait(specs, xa.in_count, this->zutex_waiter, this);

fail:
	return;
}

void ZoogVCPU::zutex_wake(xa_zutex_wake *xa_guest)
{
	xa_zutex_wake xa = *xa_guest;

	if (xa.in_n_requeue > 0)
	{
		// Ugly expedient TODO. See comment in pal_zutex.h about fixing this.
		xa_guest->out_number_woken = ((uint32_t) -1);
	}
	else
	{
		xa_guest->out_number_woken =
			vm->get_zutex_table()->wake((uint32_t) xa.in_zutex, xa.in_n_wake);
	}
}

void ZoogVCPU::get_time(xa_get_time *xa_guest)
{
	xa_guest->out_time = vm->get_alarm_thread()->get_time();
}

void ZoogVCPU::set_clock_alarm(xa_set_clock_alarm *xa_guest)
{
	xa_set_clock_alarm xa = *xa_guest;
	vm->get_alarm_thread()->reset_alarm(xa.in_time);
}

void ZoogVCPU::launch_application(xa_launch_application *xa_guest)
{
	xa_launch_application xa = *xa_guest;

	// TODO do the signature check here in the monitor, so we need only
	// pass the (already-approved) boot block to the coordinator.

	vm->lock_memory_map();

	// read out SignedBinary data structure
	SignedBinary *sb_ptr;
	sb_ptr = (SignedBinary*) vm->map_region_to_host(
			(uint32_t) xa.in_signed_binary,
			sizeof(SignedBinary));
	SAFETY_CHECK(sb_ptr != NULL);
	SignedBinary sb;
	sb = *sb_ptr;

	uint32_t sb_len;
	sb_len = Z_NTOHG(sb.cert_len) + Z_NTOHG(sb.binary_len);

	// read out entire signed_binary to pass to coordinator
	void *host_signed_binary;
	host_signed_binary = vm->map_region_to_host(
			(uint32_t) xa.in_signed_binary,
			sb_len);
	if (host_signed_binary==NULL)
	{
		vm->unlock_memory_map();	// can't coredump if we're holding this!
		vm->request_coredump();
		kablooey(true);
	}
	SAFETY_CHECK(host_signed_binary != NULL);

	vm->get_coordinator()
		->send_launch_application(host_signed_binary, sb_len);
	vm->unlock_memory_map();
fail:
	return;
}

void ZoogVCPU::endorse_me(xa_endorse_me *xa_guest)
{
	xa_endorse_me xa = *xa_guest;
	uint8_t *key_copy = NULL;
	SAFETY_CHECK(xa.in_local_key_size <= call_page_capacity - sizeof(xa));
	{
		// Copy bytes to ensure there are no races while we're parsing them
		uint8_t *key_copy = (uint8_t*) malloc(xa.in_local_key_size);
		memcpy(key_copy, xa_guest->in_local_key, xa.in_local_key_size);
		try {
			ZPubKey *app_key = new ZPubKey(key_copy, xa.in_local_key_size);

			ZCert* endorsement = new ZCert(getCurrentTime() - 1,          // Valid starting 1 second ago
										   getCurrentTime() + 31536000,   // Valid for a year
										   app_key,
										   vm->get_pub_key());
			assert(endorsement != NULL);
			endorsement->sign(vm->get_monitorKeyPair()->getPrivKey());
			assert(endorsement->size() <= call_page_capacity - sizeof(xa));
			xa_guest->out_cert_size = endorsement->size();
			endorsement->serialize(xa_guest->out_cert);
			delete endorsement;
			delete app_key;
		} catch (CryptoException ex) {
			SAFETY_CHECK(false);	// unparseable ZPubKey
		}
	}
fail:
	if (key_copy!=NULL) { free(key_copy); }
}

void ZoogVCPU::get_app_secret(xa_get_app_secret *xa_guest)
{
	xa_get_app_secret xa = *xa_guest;

	// Use the monitor's key to derive an app-specific key
	KeyDerivationKey* masterKey = vm->get_app_master_key();
	ZPubKey* appPubKey = vm->get_pub_key();
	KeyDerivationKey* appKey = masterKey->deriveKeyDerivationKey(appPubKey);

	uint32_t out_bytes_provided = xa.in_bytes_requested <= appKey->size() ? 
	                          xa.in_bytes_requested : appKey->size();
	xa_guest->out_bytes_provided = out_bytes_provided;
	assert(out_bytes_provided <= call_page_capacity - sizeof(xa));
	memcpy(xa_guest->out_bytes, appKey->getBytes(), out_bytes_provided);
}

void ZoogVCPU::get_random(xa_get_random *xa_guest)
{
	xa_get_random xa = *xa_guest;
	xa.out_bytes_provided = min(xa.in_num_bytes_requested, call_page_capacity - sizeof(xa));
	xa_guest->out_bytes_provided = xa.out_bytes_provided;
	vm->get_random_supply()
		->get_random_bytes(xa_guest->out_bytes, xa.out_bytes_provided);
}

void ZoogVCPU::internal_find_app_code(xa_internal_find_app_code *xa_guest)
{
	xa_guest->out_app_code_start = vm->get_guest_app_code_start();
	assert(xa_guest->out_app_code_start != NULL);
}

void ZoogVCPU::internal_map_alarms(xa_internal_map_alarms *xa_guest)
{
	xa_guest->out_alarms = (ZoogHostAlarms*) vm->get_alarms_guest_addr(alarm_clock);
	assert(xa_guest->out_alarms != NULL);
}

void ZoogVCPU::sublet_viewport(xa_sublet_viewport *xa_guest)
{
	vm->get_coordinator()->send_sublet_viewport(xa_guest);
}

void ZoogVCPU::repossess_viewport(xa_repossess_viewport *xa_guest)
{
	vm->get_coordinator()->send_repossess_viewport(xa_guest);
}

void ZoogVCPU::get_deed_key(xa_get_deed_key *xa_guest)
{
	vm->get_coordinator()->send_get_deed_key(xa_guest);
}

void ZoogVCPU::accept_viewport(xa_accept_viewport *xa_guest)
{
	vm->get_coordinator()->send_accept_viewport(xa_guest);
}

void ZoogVCPU::transfer_viewport(xa_transfer_viewport *xa_guest)
{
	vm->get_coordinator()->send_transfer_viewport(xa_guest);
}

void ZoogVCPU::verify_label(xa_verify_label *xa_guest)
{
	xa_verify_label xa = *xa_guest;
	bool verified = false;
	uint8_t *cert_copy = NULL;
	SAFETY_CHECK(xa.in_chain_size_bytes <= call_page_capacity - sizeof(xa));
	{
		// Copy bytes to ensure there are no races while we're parsing them
		uint8_t *cert_copy = (uint8_t*) malloc(xa.in_chain_size_bytes);
		memcpy(cert_copy, xa_guest->in_chain, xa.in_chain_size_bytes);
		try {
			ZCertChain *chain = new ZCertChain(cert_copy, xa.in_chain_size_bytes);
			verified = vm->get_pub_key()
				->verifyLabel(chain, vm->get_zoog_ca_root_key());
			lite_assert(verified);	// safe, but surprising, so complain.
			if (verified)
			{
				vm->get_coordinator()->send_verify_label();
			}
		} catch (CryptoException ex) {
			SAFETY_CHECK(false);	// unparseable ZCertChain
		}
	}
fail:
	if (cert_copy!=NULL) { free(cert_copy); }
	xa_guest->out_result = verified;
}

void ZoogVCPU::map_canvas(xa_map_canvas *xa_guest)
{
	// TODO in general, need to lock call pages to avoid races
	// where they disappear or take on another, possibly monitor-owned meaning
	// while we're on a control path that plans to poke at it.
	CanvasAcceptor ca(
		vm,
		xa_guest->in_viewport_id,
		xa_guest->in_known_formats,
		xa_guest->in_num_formats,
		&xa_guest->out_canvas);
	vm->get_coordinator()->send_map_canvas(&ca);
	ca.wait();
}

void ZoogVCPU::unmap_canvas(xa_unmap_canvas *xa_guest)
{
	vm->get_coordinator()->send_unmap_canvas(xa_guest);
}

void ZoogVCPU::update_canvas(xa_update_canvas *xa_guest)
{
	vm->get_coordinator()->send_update_canvas(xa_guest);
}

void ZoogVCPU::receive_ui_event(xa_receive_ui_event *xa_guest)
{
	vm->get_coordinator()->dequeue_ui_event(&xa_guest->out_event);
}

void ZoogVCPU::debug_set_idt_handler(xa_debug_set_idt_handler *xa_guest)
{
	vm->set_idt_handler(xa_guest->in_handler);
}

void ZoogVCPU::get_registers(Core_x86_registers *core_regs)
{
	int rc;
	struct kvm_regs regs;
	struct kvm_sregs sregs;
	
	if (vcpu_allocation==NULL)
	{
		regs = snap_regs;
		sregs = snap_sregs;
	}
	else
	{
		rc = ioctl(vcpufd(), KVM_GET_REGS, &regs);
		assert(rc==0);
		rc = ioctl(vcpufd(), KVM_GET_SREGS, &sregs);
		assert(rc==0);
	}

	core_regs->bx = regs.rbx;
	core_regs->cx = regs.rcx;
	core_regs->dx = regs.rdx;
	core_regs->si = regs.rsi;
	core_regs->di = regs.rdi;
	core_regs->bp = regs.rbp;
	core_regs->ax = regs.rax;
	core_regs->ds = sregs.ds.selector;
	core_regs->es = sregs.es.selector;
	core_regs->fs = sregs.fs.selector;
	core_regs->gs = sregs.gs.selector;
	core_regs->orig_ax= regs.rax;
	core_regs->ip = regs.rip;
	core_regs->cs = sregs.cs.selector;
	core_regs->flags = regs.rflags;
	core_regs->sp = regs.rsp;
	core_regs->ss = sregs.ss.selector;

	printf("Control registers: CR0=%llx, CR2=%llx, CR3=%llx, CR4=%llx, CR8=%llx, efer=%llx\n", sregs.cr0, sregs.cr2, sregs.cr3, sregs.cr4, sregs.cr8, sregs.efer);
}

bool ZoogVCPU::vcpu_held()
{
	return (vcpu_allocation != NULL);
}

int ZoogVCPU::vcpu_yield_timeout_ms()
{
	return 50;
}

void ZoogVCPU::vcpu_yield()
{
	yield_kvm_vcpu();
}
