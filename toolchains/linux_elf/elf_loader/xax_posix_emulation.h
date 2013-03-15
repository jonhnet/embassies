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

#include "linux_syscall_ifc.h"
#include "handle_table.h"
#include "xax_stack_table.h"
#include "xerrfs.h"
#include "xmaskfs.h"
#include "strace.h"
#include "fake_ids.h"
#include "xax_skinny_network.h"
#include "thread_table.h"
#include "xbrk.h"
#include "zoog_malloc_factory.h"
#include "SyncFactory.h"
#include "XaxVFS.h"
#include "XVFSNamespace.h"
#include "malloc_trace.h"
#include "ElfArgs.h"
#include "perf_measure.h"

class XVFSHandleWrapper;

struct s_xax_posix_emulation;
typedef struct s_xax_posix_emulation XaxPosixEmulation;
class ZLCVFS_Wrapper;

struct s_xax_posix_emulation {
	int _initted;
	int network_initted;

	FakeIds fake_ids;

	XVFSNamespace *xvfs_namespace;
	HandleTable *handle_table;
	ZoogDispatchTable_v1 *zdt;
	XaxStackTable stack_table;
	ThreadTable thread_table;
	void *thread0_initial_gs;

	ZoogMallocFactory zmf;
	MallocFactory *mf;

	MallocTraceFuncs *mtf;

	XerrHandle *stdout_handle;
	XerrHandle *stderr_handle;

	XaxSkinnyNetwork *xax_skinny_network;

	Xbrk xbrk;

	Strace strace;
	PerfMeasure *perf_measure;
	bool strace_flag;
	bool mmap_flag;

	uint32_t *exit_zutex;

	ZLCVFS_Wrapper *zlcvfs_wrapper;
	const char *vendor_name_string;
	debug_cpu_times_f *debug_cpu_times;
};

void xpe_init(XaxPosixEmulation *xpe, ZoogDispatchTable_v1 *zdt, uint32_t stack_base, StartupContextFactory *scf);

void xpe_mount(XaxPosixEmulation *xpe, const char *prefix, XaxVFSIfc *xfs);
void xnull_fstat64(XfsErr *err, struct stat64 *buf, size_t size);
uint32_t xpe_dispatch(XaxPosixEmulation *xpe, UserRegs *ur);

// shared by xi_network
XVFSMount *_xpe_map_path_to_mount(XaxPosixEmulation *xpe, const char *file, XfsPath *out_path);
void _xpe_path_free(XfsPath *path);
XVFSHandleWrapper *xpe_open_hook_handle(XaxPosixEmulation *xpe, XfsErr *err, const char *path, int oflag, XVOpenHooks *xoh);
uint32_t xpe_open_hook_fd(XaxPosixEmulation *xpe, const char *path, int oflag, XVOpenHooks *xoh);
int xi_close(XaxPosixEmulation *xpe, int filenum);

// reused by xoverlayfs
void xpe_stat64(XaxPosixEmulation *xpe, XfsErr *err, const char *path, struct stat64 *buf);

