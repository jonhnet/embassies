#include <alloca.h>
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

#include "VCPUPool.h"
#include "ZoogVM.h"

VCPUAllocation* VCPUAllocation::create(ZoogVM *vm, int vcpu_ident)
{
	int _vcpufd = ioctl(vm->get_vmfd(), KVM_CREATE_VCPU, vcpu_ident);
	if (_vcpufd==-1 && errno==EINVAL)
	{
		fprintf(stderr, "Out of VCPUs\n");
		return NULL;
	}
	assert(_vcpufd>=0);

	set_up_cpuid_1(_vcpufd);

	return new VCPUAllocation(vm, _vcpufd);
}

void VCPUAllocation::set_up_cpuid_1(int _vcpufd)
{
	// Since cpuid is a constant, we'll not bother saving/restoring
	// it in VCPU; we'll just set it up once for each VCPU we create.

	int size = sizeof(struct kvm_cpuid) + sizeof(struct kvm_cpuid_entry);
	void* space = alloca(size);
	memset(space, 0, size);
	struct kvm_cpuid* kc = (struct kvm_cpuid*) space;
	struct kvm_cpuid_entry* entry = (struct kvm_cpuid_entry*) &kc[1];
	get_host_cpuid_func_1(entry);
	lite_assert((entry->edx & (1<<26))!=0);
	kc->nent = 1;
	kc->padding = 0;
	ioctl(_vcpufd, KVM_SET_CPUID, kc);
}

void VCPUAllocation::get_host_cpuid_func_1(struct kvm_cpuid_entry *ent)
{
	ent->function = 1;
	asm volatile (
		 "movl $0x1, %%eax;"
		 "pushl %%edx;"
		 "cpuid;"
		 "popl %%edx;"
		 "movl %%eax, %0;"
		 "movl %%ebx, %1;"
		 : "=g" (ent->eax),
		   "=g" (ent->ebx)
		 :
		 : "%eax", "%ebx", "%ecx"
		 );
	asm volatile (
		 "movl $0x1, %%eax;"
		 "pushl %%ebx;"
		 "cpuid;"
		 "popl %%ebx;"
		 "movl %%ecx, %0;"
		 "movl %%edx, %1;"
		 : "=g" (ent->ecx),
		   "=g" (ent->edx)
		 :
		 : "%eax", "%ecx", "%edx"
		 );
}

VCPUAllocation::VCPUAllocation(ZoogVM *vm, int _vcpufd)
	: vcpufd(_vcpufd)
{
	int mmap_size = ioctl(vm->get_kvmfd(), KVM_GET_VCPU_MMAP_SIZE, 0);
	assert(mmap_size>0);

	kvm_run = (struct kvm_run*) mmap(NULL, mmap_size, PROT_READ|PROT_WRITE, MAP_SHARED, vcpufd, 0);
	assert(kvm_run!=MAP_FAILED);
}

VCPUAllocation::~VCPUAllocation()
{
	close(vcpufd);
}

//////////////////////////////////////////////////////////////////////////////

VCPUPool::VCPUPool(ZoogVM *vm)
	: all_vcpus_used(false)
{
	this->vm = vm;
	linked_list_init(&free_list, vm->get_malloc_factory());
	mutex = vm->get_sync_factory()->new_mutex(false);
	vcpu_freed_event = vm->get_sync_factory()->new_event(false);
	next_free_vcpu_ident = 0;
}

VCPUAllocation *VCPUPool::allocate()
{
	VCPUAllocation *result;
	mutex->lock();
	while (true)
	{
		if (free_list.count>0)
		{
			result = (VCPUAllocation*) linked_list_remove_head(&free_list);
			// fprintf(stderr, "VCPUPool recycles a kvm VCPU\n");
			break;
		}
		else if (!all_vcpus_used)	// none free, but maybe can create one
		{
			result = VCPUAllocation::create(vm, next_free_vcpu_ident);
			if (result!=NULL)
			{
				// fprintf(stderr, "VCPUPool allocates kvm VCPU #%d\n", next_free_vcpu_ident);
				next_free_vcpu_ident+=1;
				break;
			}
			else
			{
				all_vcpus_used = true;
				// loop around to fall through to wait case
				continue;
			}
		}
		else
		{
			// fprintf(stderr, "Waiting on a free VCPU.\n");
			vcpu_freed_event->reset();
			mutex->unlock();
			vcpu_freed_event->wait();
			mutex->lock();
			continue;
		}
	}
	mutex->unlock();
	return result;
}

void VCPUPool::free(VCPUAllocation *vcpu_allocation)
{
	mutex->lock();
	bool need_wake = (free_list.count==0);
	linked_list_insert_head(&free_list, vcpu_allocation);
	if (need_wake)
	{
		vcpu_freed_event->signal();
	}
	mutex->unlock();
}
