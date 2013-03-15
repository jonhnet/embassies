/* Copyright (c) Microsoft Corporation                                       */
/*                                                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* Licensed under the Apache License, Version 2.0 (the "License"); you may   */
/* not use this file except in compliance with the License.  You may obtain  */
/* a copy of the License at http://www.apache.org/licenses/LICENSE-2.0       */
/*                                                                           */
/* THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS     */
/* OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION      */
/* ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR   */
/* PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.                              */
/*                                                                           */
/* See the Apache Version 2.0 License for specific language governing        */
/* permissions and limitations under the License.                            */
/*                                                                           */
#pragma once

#include "linked_list.h"
#include "SyncFactory.h"

class ZoogVM;

class VCPUAllocation
{
public:
	~VCPUAllocation();
		// if we ever decide to contract vcpu allocation. But I don't think
		// we can reexpand, so we don't ever actually do this.
		// (If we could recycle kvm vcpus, we wouldn't need this pool class!)

	static VCPUAllocation* create(ZoogVM *vm, int vcpu_ident);
	static void set_up_cpuid_1(int _vcpufd);
	static void get_host_cpuid_func_1(struct kvm_cpuid_entry *ent);
	inline int get_vcpufd() { return vcpufd; }
	struct kvm_run *get_kvm_run() { return kvm_run; }

private:
	VCPUAllocation(ZoogVM *vm, int _vcpufd);

	int vcpufd;
	struct kvm_run *kvm_run;
};

class VCPUPool
{
public:
	VCPUPool(ZoogVM *vm);
	VCPUAllocation *allocate();
	void free(VCPUAllocation *vcpu_allocation);

private:
	ZoogVM *vm;
	int next_free_vcpu_ident;
	bool all_vcpus_used;
	SyncFactoryMutex* mutex;
	SyncFactoryEvent* vcpu_freed_event;
	LinkedList free_list;
};
