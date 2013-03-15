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
| PAL_ABI
|
| Purpose: Platform Abstraction Layer Application Binary Interface
----------------------------------------------------------------------*/

#pragma once

#include "pal_abi/pal_types.h"
#include "pal_abi/pal_process.h"
#include "pal_abi/pal_zutex.h"
#include "pal_abi/pal_net.h"
#include "pal_abi/pal_time.h"
#include "pal_abi/pal_ui.h"
#include "pal_abi/pal_crypto.h"

/*----------------------------------------------------------------------
| Struct: ZoogDispatchTable_v1
|
| Purpose:  TODO
----------------------------------------------------------------------*/
typedef struct {

// pal_process.h
    // Debug hooks
    zf_zoog_lookup_extension		zoog_lookup_extension;
    // Allocate memory
    zf_zoog_allocate_memory			zoog_allocate_memory;
    // Free memory
    zf_zoog_free_memory				zoog_free_memory;
    // Create a new thread within this process
    zf_zoog_thread_create			zoog_thread_create;
    // Exit the current thread
    zf_zoog_thread_exit				zoog_thread_exit;
    // Set the fs, gs segment registers (x86 arch specific)
    zf_zoog_x86_set_segments		zoog_x86_set_segments;
    // Exit this process, returning its resources
    zf_zoog_exit					zoog_exit;
    // Launch a new process, identifying a new principal
    zf_zoog_launch_application		zoog_launch_application;


// pal_crypto.h
    // Issue a certificate binding the supplied key to the principal's identity
    zf_zoog_endorse_me          	zoog_endorse_me;
    // Retrieve a kernel-supplied (and hence, platform-specific), app-specific secret value
    zf_zoog_get_app_secret      	zoog_get_app_secret;
		// Obtain some strong random values from the monitor
 	zf_zoog_get_random          	zoog_get_random;


// pal_zutex.h
    // Wait until one of a set of zutexes is signaled
    zf_zoog_zutex_wait				zoog_zutex_wait;
    // Wake an application-defined zutex
    zf_zoog_zutex_wake				zoog_zutex_wake;
	// Retrieve the set of alarms (host-defined zutexes)
	zf_zoog_get_alarms				zoog_get_alarms;

// pal_net.h
    // TODO
    zf_zoog_get_ifconfig			zoog_get_ifconfig;
    // TODO
    zf_zoog_alloc_net_buffer		zoog_alloc_net_buffer;
    // TODO
    zf_zoog_send_net_buffer			zoog_send_net_buffer;
    // TODO
    zf_zoog_receive_net_buffer		zoog_receive_net_buffer;
    // TODO
    zf_zoog_free_net_buffer			zoog_free_net_buffer;


// pal_time.h
	// Retrieve the current real-time clock
	zf_zoog_get_time				zoog_get_time;
	// Set the real-time alarm to trigger a zutex
	zf_zoog_set_clock_alarm			zoog_set_clock_alarm;


// pal_ui.h
	// landlord calls
	zf_zoog_sublet_viewport		zoog_sublet_viewport;
	zf_zoog_repossess_viewport	zoog_repossess_viewport;
	zf_zoog_get_deed_key		zoog_get_deed_key;
	// tenant calls
	zf_zoog_accept_viewport		zoog_accept_viewport;
	zf_zoog_transfer_viewport	zoog_transfer_viewport;
	zf_zoog_verify_label		zoog_verify_label;
	zf_zoog_map_canvas			zoog_map_canvas;
	zf_zoog_unmap_canvas		zoog_unmap_canvas;
	zf_zoog_update_canvas		zoog_update_canvas;
	zf_zoog_receive_ui_event	zoog_receive_ui_event;
} ZoogDispatchTable_v1;


/*----------------------------------------------------------------------
| Define: XAX_SUCCESS
|
| Purpose:  TODO
----------------------------------------------------------------------*/
#define XAX_SUCCESS					0

/*----------------------------------------------------------------------
| Define: XAX_UNSPECIFIED_ERROR
|
| Purpose:  TODO
----------------------------------------------------------------------*/
#define XAX_UNSPECIFIED_ERROR		10
// TODO: find all instances and see if we can give them sane names.

// Note that a PAL's implementation of each of these functions requires
// some amount of stack available, which the app must supply. We haven't
// specified the value of "some", or tested apps or PALs for compliance.
// It'll probably be sufficient to declare some small, "reasonable" value,
// like 1K: a value big enough any sane PAL could operate within it,
// but small enough that it's not a great burden on apps.
// Or the PAL could expose an interface to make the declaration dynamic.
// Or the PAL could expose even more detail, per-call, so that threads that
// only need to call certain PAL functions can be extra miserly.
// But this whole idea doesn't seem very simple in specification. So we're
// gonna go with the constant. We just haven't said which one yet.
