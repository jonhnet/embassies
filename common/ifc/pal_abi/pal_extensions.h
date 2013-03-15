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
/*----------------------------------------------------------------------
| PAL_EXTENSIONS
|
| Purpose: Process Abstraction Layer Debugging Extensions
|
| Each extension is defined by a _name string and a function pointer type.
| An application can ask for the extension via PAL ABI function
| zoog_lookup_extension. If available, the ABI returns the corresponding
| function pointer; otherwise, it returns NULL.
|
| All sane programs should work correctly with no extensions present;
| no trusted CEI implementation should implement any extensions.
|
|	TODO some of these types are function types, others are pointers. Fix it.
|
----------------------------------------------------------------------*/

#pragma once

#include "pal_abi/pal_basic_types.h"

/*----------------------------------------------------------------------
| Function Signature: debug_logfile_append_f
|
| Purpose: Append a message to the logfile
|
| Parameters:
|   logfile_name (IN) -- Name of the logifle
|   message      (IN) -- Message to append
|
| Returns: none
----------------------------------------------------------------------*/
typedef void (debug_logfile_append_f)(const char *logfile_name, const char *message);
#define DEBUG_LOGFILE_APPEND_NAME "debug_logfile_append"


/*----------------------------------------------------------------------
| Function Signature: debug_alloc_protected_memory_f
|
| Purpose: Allocated protected memory
|
| Parameters:
|   length (IN) -- Size to allocate, in bytes
|
| Returns: Allocated memory
----------------------------------------------------------------------*/
typedef void *(debug_alloc_protected_memory_f)(size_t length);
#define DEBUG_ALLOC_PROTECTED_MEMORY_NAME "debug_alloc_protected_memory"


/*----------------------------------------------------------------------
| Function Signature: debug_announce_thread_f
|
| Purpose: TODO
|
| Parameters: none
|
| Returns: none
----------------------------------------------------------------------*/
typedef void (debug_announce_thread_f)(void);
#define DEBUG_ANNOUNCE_THREAD_NAME "debug_announce_thread"


/*----------------------------------------------------------------------
| Function Signature: debug_new_profiler_epoch_f
|
| Purpose: TODO
|
| Parameters: 
|   name_of_last_epoch (IN) - TODO
|
| Returns: none
----------------------------------------------------------------------*/
typedef void (debug_new_profiler_epoch_f)(const char *name_of_last_epoch);
#define DEBUG_NEW_PROFILER_EPOCH_NAME "debug_new_profiler_epoch"

/*----------------------------------------------------------------------
| Function Signature: debug_get_link_mtu_f
|
| Purpose: leak out information about the implementation constraints on
| link mtu. Stand-in for eventual link-mtu-discovery.
|
| Parameters: 
|   none
|
| Returns: link IP mtu
----------------------------------------------------------------------*/
typedef uint32_t (debug_get_link_mtu_f)();
#define DEBUG_GET_LINK_MTU_NAME "debug_get_link_mtu"

typedef ViewportID (debug_create_toplevel_window_f)();
#define DEBUG_CREATE_TOPLEVEL_WINDOW_NAME "debug_create_toplevel_window"

/*----------------------------------------------------------------------
| Function Signature: debug_trace_touched_pages_f
|
| Purpose: Allocates a file that should be traced for access
|
| Parameters:
|   addr (IN) -- Start of the file
|   length (IN) -- Size allocated, in bytes
|
| Returns: Nothing
----------------------------------------------------------------------*/
typedef void (*debug_trace_touched_pages_f)(uint8_t* addr, size_t length);
#define DEBUG_TRACE_TOUCHED_PAGES "debug_allocated_pages"

/*----------------------------------------------------------------------
| Function Signature: debug_cpu_times_f
|
| Purpose: Return the POSIX 'times()' syscall values. Used for
|   perf debugging.
|
| Parameters:
|   addr (IN) -- Start of the file
|   length (IN) -- Size allocated, in bytes
|
| Returns: Nothing
----------------------------------------------------------------------*/
struct tms;	// #include <sys/times.h> if you want to know what's in there. :v)
typedef int (debug_cpu_times_f)(struct tms *buf);
#define DEBUG_CPU_TIMES "debug_cpu_times"
