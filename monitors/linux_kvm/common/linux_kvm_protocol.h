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

// Protocol between sandboxed (KVM-bound) app and monitor process.

#include "pal_abi/pal_abi.h"

typedef enum XP_Opcode {
	xp_allocate_memory=0x20090001,
	xp_free_memory,
	xp_get_ifconfig,
	xp_alloc_net_buffer,
	xp_send_net_buffer,
	xp_receive_net_buffer,
	xp_free_net_buffer,
	xp_thread_create,
	xp_thread_exit,
	xp_x86_set_segments,
	xp_zutex_wait,
	xp_zutex_wake,
	xp_get_time,
	xp_set_clock_alarm,
	xp_exit,
	xp_launch_application,
	xp_endorse_me,
	xp_get_app_secret,
	xp_get_random,
	xp_extn_debug_logfile_append,
	xp_extn_debug_create_toplevel_window,
	xp_internal_find_app_code,
	xp_internal_map_alarms,
	xp_sublet_viewport,
	xp_repossess_viewport,
	xp_get_deed_key,
	xp_accept_viewport,
	xp_transfer_viewport,
	xp_verify_label,
	xp_map_canvas,
	xp_unmap_canvas,
	xp_update_canvas,
	xp_receive_ui_event,
	xp_debug_set_idt_handler,
} XP_Opcode;

typedef struct {
	XP_Opcode opcode;
	uint32_t in_length;
	void *out_pointer;
} xa_allocate_memory;

typedef struct {
	XP_Opcode opcode;
	void *in_pointer;
	uint32_t in_length;
} xa_free_memory;

typedef struct {
	XP_Opcode opcode;
	uint32_t out_count;
	XIPifconfig out_ifconfigs[8];	// arbitrary limit
} xa_get_ifconfig;

typedef struct {
	XP_Opcode opcode;
	uint32_t in_payload_size;
	ZoogNetBuffer *out_zoog_net_buffer;
} xa_alloc_net_buffer;

typedef struct {
	XP_Opcode opcode;
	ZoogNetBuffer *in_zoog_net_buffer;
	bool in_release;
} xa_send_net_buffer;

typedef struct {
	XP_Opcode opcode;
	ZoogNetBuffer *out_zoog_net_buffer;
} xa_receive_net_buffer;

typedef struct {
	XP_Opcode opcode;
	ZoogNetBuffer *in_zoog_net_buffer;
} xa_free_net_buffer;

typedef struct {
	XP_Opcode opcode;
	zoog_thread_start_f *in_func;
	void *in_stack;
	bool out_result;
} xa_thread_create;

typedef struct {
	XP_Opcode opcode;
} xa_thread_exit;

typedef struct {
	XP_Opcode opcode;
} xa_exit;

typedef struct {
	XP_Opcode opcode;
	uint32_t in_fs;
	uint32_t in_gs;
} xa_x86_set_segments;

typedef struct {
	XP_Opcode opcode;
	uint32_t in_count;
	ZutexWaitSpec in_specs[0];
} xa_zutex_wait;

typedef struct {
	XP_Opcode opcode;
	uint32_t *in_zutex;
	// uint32_t match_val	//  I think this is unused
	Zutex_count in_n_wake;
	Zutex_count in_n_requeue;
	uint32_t *in_requeue_zutex;
	uint32_t in_requeue_match_val;
	uint32_t out_number_woken;
} xa_zutex_wake;

typedef struct {
	XP_Opcode opcode;
	uint64_t out_time;
} xa_get_time;

typedef struct {
	XP_Opcode opcode;
	uint64_t in_time;
} xa_set_clock_alarm;

typedef struct {
	XP_Opcode opcode;
	SignedBinary *in_signed_binary;
} xa_launch_application;

typedef struct {
	XP_Opcode opcode;
	uint32_t in_local_key_size;
	uint32_t out_cert_size;
	union {
		uint8_t in_local_key[0];
		uint8_t out_cert[0];
	};
} xa_endorse_me;

typedef struct {
	XP_Opcode opcode;
	uint32_t in_bytes_requested;
	uint32_t out_bytes_provided;
	uint8_t out_bytes[0];
} xa_get_app_secret;

typedef struct {
	XP_Opcode opcode;
	uint32_t in_num_bytes_requested;
	uint32_t out_bytes_provided;
	uint8_t out_bytes[0];
} xa_get_random;

typedef struct {
	XP_Opcode opcode;
	uint32_t in_msg_len;
	char in_logfile_name[32];
	char in_msg_bytes[0];
} xa_extn_debug_logfile_append;

typedef struct {
	XP_Opcode opcode;
	ViewportID out_viewport_id;
} xa_extn_debug_create_toplevel_window;

typedef struct {
	XP_Opcode opcode;
	void *out_app_code_start;
} xa_internal_find_app_code;

typedef struct {
	XP_Opcode opcode;
	ZoogHostAlarms *out_alarms;
} xa_internal_map_alarms;

typedef struct {
	XP_Opcode opcode;
	ViewportID in_tenant_viewport;
	ZRectangle in_rectangle;
	ViewportID out_landlord_viewport;
	Deed out_deed;
} xa_sublet_viewport;

typedef struct {
	XP_Opcode opcode;
	ViewportID in_landlord_viewport;
} xa_repossess_viewport;

typedef struct {
	XP_Opcode opcode;
	ViewportID in_landlord_viewport;
	DeedKey out_deed_key;
} xa_get_deed_key;

typedef struct {
	XP_Opcode opcode;
	Deed in_deed;
	ViewportID out_tenant_viewport;
	DeedKey out_deed_key;
} xa_accept_viewport;

typedef struct {
	XP_Opcode opcode;
	ViewportID in_tenant_viewport;
	Deed out_deed;
} xa_transfer_viewport;

typedef struct {
	XP_Opcode opcode;
	bool out_result;
	uint32_t in_chain_size_bytes;
	uint8_t in_chain[0];
} xa_verify_label;

typedef struct {
	XP_Opcode opcode;
	ViewportID in_viewport_id;
	PixelFormat in_known_formats[4];
	int in_num_formats;
	ZCanvas out_canvas;
} xa_map_canvas;

typedef struct {
	XP_Opcode opcode;
	ZCanvasID in_canvas_id;
} xa_unmap_canvas;

typedef struct {
	XP_Opcode opcode;
	ZCanvasID in_canvas_id;
	ZRectangle in_rectangle;
} xa_update_canvas;

typedef struct {
	XP_Opcode opcode;
	ZoogUIEvent out_event;
} xa_receive_ui_event;


typedef struct {
	XP_Opcode opcode;
	uint32_t in_handler;
} xa_debug_set_idt_handler;
