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

#include <stdint.h>
#include <sys/types.h>

#include "pal_abi/pal_abi.h"

typedef struct s_xax_port_buffer {
	uint32_t buffer_handle;
	uint32_t capacity;
	uint32_t dbg_mapping_size;
	bool dbg_mapped_in_client;
	bool dbg_owned_by_client;
	union {
		struct {
			ZoogNetBuffer znb_header;
			uint8_t _data[];
		} znb;
		struct {
			ZCanvas canvas;
				// data pointer field undefined, since monitor doesn't
				// know where this object gets mapped in PAL. PAL should
				// point it at _data:
			uint8_t _data[];
		} canvas;
	} un;
} XaxPortBuffer;

typedef enum {
	xpConnect,
	xpDisconnect,
	xpXNBAlloc,
	xpXNBRecv,
	xpXNBSend,
	xpXNBRelease,
	xpGetIfconfig,
	xpZutexWaitPrepare,
	xpZutexWaitCommit,
	xpZutexWaitAbort,
	xpZutexWake,
	xpLaunch,
	xpEndorseMe,
	xpGetAppSecret,
	xpEventPoll,
	xpSubletViewport,
	xpRepossessViewport,
	xpGetDeedKey,
	xpAcceptViewport,
	xpTransferViewport,
	xpVerifyLabel,
	xpMapCanvas,
	xpUnmapCanvas,
	xpUpdateCanvas,
	xpReceiveUIEvent,
	xpDebugCreateToplevelWindow,
	xpGetRandom
} XaxPortOpcode;


struct XpReqHeader {
	uint32_t message_len;
	uint32_t opcode;
	uint32_t process_id;
};

struct XpReplyHeader {
	uint32_t message_len;
};


struct XpReqConnect : public XpReqHeader {
//	bool grant_toplevel_window;
	uint32_t pub_key_len;
	uint8_t pub_key[0];

	static const bool fixed_length = false;
	static const XaxPortOpcode OPCODE = xpConnect;
};

struct XpReplyConnect : public XpReplyHeader { };


struct XpReqDisconnect : public XpReqHeader {
	static const bool fixed_length = true;
	static const XaxPortOpcode OPCODE = xpDisconnect;
};

struct XpReplyDisconnect : public XpReplyHeader { };


#define INVALID_HANDLE	((uint32_t) -1)

typedef struct {
	char path[256];
	uint32_t length;
	uint32_t deprecates_old_handle;
		// this is how the server informs the pal side that it has reclaimed
		// a slot.
} MapFileSpec;

typedef struct {
	int key;
	uint32_t length;
	uint32_t deprecates_old_handle;
		// this is how the server informs the pal side that it has reclaimed
		// a slot.
} ShmgetKeySpec;

typedef enum {
	use_open_buffer = 1,
	use_map_file_spec = 2,
	use_shmget_key = 3,
	no_packet_ready = 4
} BufferReplyType;

typedef union {
	uint32_t open_buffer_handle;
	MapFileSpec map_file_spec;
	ShmgetKeySpec shmget_key_spec;
} MapSpec;

struct XpBufferReply : public XpReplyHeader {
	BufferReplyType type;
	MapSpec map_spec;
};


struct XpReqXNBAlloc : public XpReqHeader {
	uint32_t thread_id;
	uint32_t payload_size;

	static const bool fixed_length = true;
	static const XaxPortOpcode OPCODE = xpXNBAlloc;
};

struct XpReplyXNBAlloc : public XpBufferReply { };


struct XpReqXNBRecv : public XpReqHeader {
	uint32_t thread_id;

	static const bool fixed_length = true;
	static const XaxPortOpcode OPCODE = xpXNBRecv;
};

struct XpReplyXNBRecv : public XpBufferReply { };


struct XpReqXNBSend : public XpReqHeader {
	uint32_t open_buffer_handle;
	bool release;

	static const bool fixed_length = true;
	static const XaxPortOpcode OPCODE = xpXNBSend;
};

struct XpReplyXNBSend : public XpReplyHeader { };


struct XpReqXNBRelease : public XpReqHeader {
	uint32_t open_buffer_handle;
	bool discarded_at_pal;

	static const bool fixed_length = true;
	static const XaxPortOpcode OPCODE = xpXNBRelease;
};

struct XpReplyXNBRelease : public XpReplyHeader { };


struct XpReqGetIfconfig : public XpReqHeader {
	static const bool fixed_length = true;
	static const XaxPortOpcode OPCODE = xpGetIfconfig;
};

struct XpReplyGetIfconfig : public XpReplyHeader {
	int count;
	XIPifconfig ifconfig_out[2];
};


// zutexes limited to XaxPortRPC because monitor
// can't see into child's address space (without being
// its parent or ptracing; if we're going to ptrace,
// well, it's finally time to scrap xax_port altogether.)
// TODO why are zutexes managed in monitor? Seems like pal process
// could implement them internally.
struct XpReqZutexWait : public XpReqHeader {
	uint32_t thread_id;
	uint32_t count;
	uint32_t *specs[0];
};

struct XpReqZutexWaitPrepare : public XpReqZutexWait {
	static const bool fixed_length = false;
	static const XaxPortOpcode OPCODE = xpZutexWaitPrepare;
};

struct XpReqZutexWaitCommit : public XpReqZutexWait {
	static const bool fixed_length = false;
	static const XaxPortOpcode OPCODE = xpZutexWaitCommit;
};

struct XpReqZutexWaitAbort : public XpReqZutexWait {
	static const bool fixed_length = false;
	static const XaxPortOpcode OPCODE = xpZutexWaitAbort;
};
	// Prepare locks the specs
	// Commit and Abort unlock the specs

struct XpReplyZutexWait : public XpReplyHeader { };
struct XpReplyZutexWaitPrepare : public XpReplyZutexWait { };
struct XpReplyZutexWaitCommit : public XpReplyZutexWait { };
struct XpReplyZutexWaitAbort : public XpReplyZutexWait { };

struct XpReqZutexWake : public XpReqHeader {
	uint32_t thread_id;
	uint32_t *zutex;
	uint32_t match_val;
	Zutex_count n_wake;
	Zutex_count n_requeue;
	uint32_t *requeue_zutex;
	uint32_t requeue_match_val;

	static const bool fixed_length = false;
	static const XaxPortOpcode OPCODE = xpZutexWake;
};

struct XpReplyZutexWake : public XpReplyHeader {
	bool success;
};


struct XpReqLaunch : public XpReqHeader {
	uint32_t open_buffer_handle;

	static const bool fixed_length = true;
	static const XaxPortOpcode OPCODE = xpLaunch;
};

struct XpReplyLaunch : public XpReplyHeader { };


struct XpReqEndorseMe : public XpReqHeader {
	uint32_t app_key_bytes_len;
	uint8_t  app_key_bytes[0];

	static const bool fixed_length = false;
	static const XaxPortOpcode OPCODE = xpEndorseMe;
};

struct XpReplyEndorseMe : public XpReplyHeader {
	uint32_t app_cert_bytes_len;
	uint8_t  app_cert_bytes[0];
};


struct XpReqGetAppSecret : public XpReqHeader {
	uint32_t num_bytes_requested;

	static const bool fixed_length = true;
	static const XaxPortOpcode OPCODE = xpGetAppSecret;
};

struct XpReplyGetAppSecret : public XpReplyHeader {
	uint32_t num_bytes_returned;
	uint8_t  secret_bytes[0];
};

struct XpReqGetRandom : public XpReqHeader {
	uint32_t num_bytes_requested;

	static const bool fixed_length = true;
	static const XaxPortOpcode OPCODE = xpGetRandom;
};

struct XpReplyGetRandom : public XpReplyHeader {
	uint32_t num_bytes_returned;
	uint8_t  random_bytes[0];
};

struct XpReqEventPoll : public XpReqHeader {
	uint32_t known_sequence_numbers[alarm_count];

	static const bool fixed_length = true;
	static const XaxPortOpcode OPCODE = xpEventPoll;
};

struct XpReplyEventPoll : public XpReplyHeader {
	uint32_t new_sequence_numbers[alarm_count];
};

struct XpReqSubletViewport : public XpReqHeader {
	ViewportID tenant_viewport;
	ZRectangle rectangle;

	static const bool fixed_length = true;
	static const XaxPortOpcode OPCODE = xpSubletViewport;
};

struct XpReplySubletViewport : public XpReplyHeader {
	ViewportID out_landlord_viewport;
	Deed out_deed;
};

struct XpReqRepossessViewport : public XpReqHeader {
	ViewportID landlord_viewport;

	static const bool fixed_length = true;
	static const XaxPortOpcode OPCODE = xpRepossessViewport;
};

struct XpReplyRepossessViewport : public XpReplyHeader { };

struct XpReqGetDeedKey : public XpReqHeader {
	ViewportID landlord_viewport;

	static const bool fixed_length = true;
	static const XaxPortOpcode OPCODE = xpGetDeedKey;
};

struct XpReplyGetDeedKey : public XpReplyHeader {
	DeedKey out_deed_key;
};

struct XpReqAcceptViewport : public XpReqHeader {
	Deed deed;

	static const bool fixed_length = true;
	static const XaxPortOpcode OPCODE = xpAcceptViewport;
};

struct XpReplyAcceptViewport : public XpReplyHeader {
	ViewportID tenant_viewport;
	DeedKey deed_key;
};

struct XpReqTransferViewport : public XpReqHeader {
	ViewportID tenant_viewport;

	static const bool fixed_length = true;
	static const XaxPortOpcode OPCODE = xpTransferViewport;
};

struct XpReplyTransferViewport : public XpReplyHeader {
	Deed deed;
};

struct XpReqMapCanvas : public XpReqHeader {
	uint32_t thread_id;
		// needed in case the XPB pool is full and we block in monitor.
	ViewportID viewport_id;
	PixelFormat known_formats[4];
	int num_formats;

	static const bool fixed_length = true;
	static const XaxPortOpcode OPCODE = xpMapCanvas;
};

struct XpReplyMapCanvas : public XpBufferReply { };


struct XpReqUnmapCanvas : public XpReqHeader {
	ZCanvasID canvas_id;

	static const bool fixed_length = true;
	static const XaxPortOpcode OPCODE = xpUnmapCanvas;
};

struct XpReplyUnmapCanvas : public XpReplyHeader { };

struct XpReqVerifyLabel : public XpReqHeader {
	uint8_t    cert_chain_bytes[0];

	static const bool fixed_length = false;
	static const XaxPortOpcode OPCODE = xpVerifyLabel;
};

struct XpReplyVerifyLabel : public XpReplyHeader {
	bool success;
};


struct XpReqUpdateCanvas : public XpReqHeader {
	ZCanvasID canvas_id;
	ZRectangle rectangle;

	static const bool fixed_length = true;
	static const XaxPortOpcode OPCODE = xpUpdateCanvas;
};

struct XpReplyUpdateCanvas : public XpReplyHeader { };

struct XpReqReceiveUIEvent : public XpReqHeader {
	static const bool fixed_length = true;
	static const XaxPortOpcode OPCODE = xpReceiveUIEvent;
};

struct XpReplyReceiveUIEvent : public XpReplyHeader {
	ZoogUIEvent event;
};

struct XpReqDebugCreateToplevelWindow : public XpReqHeader {
	static const bool fixed_length = true;
	static const XaxPortOpcode OPCODE = xpDebugCreateToplevelWindow;
};

struct XpReplyDebugCreateToplevelWindow : public XpReplyHeader {
	ViewportID tenant_id;
};

#if 0
struct XpReqDelegateViewport : public XpReqHeader {
	ZCanvasID canvas_id;
	uint32_t app_pub_key_bytes_len;
	uint8_t  app_pub_key_bytes[0];

	static const bool fixed_length = false;
	static const XaxPortOpcode OPCODE = xpDelegateViewport;
};

struct XpReplyDelegateViewport : public XpReplyHeader {
	ViewportID viewport_id;
};

struct XpReqUpdateViewport : public XpReqHeader {
	ViewportID viewport_id;
	uint32_t x;
	uint32_t y;
	uint32_t width;
	uint32_t height;

	static const bool fixed_length = true;
	static const XaxPortOpcode OPCODE = xpUpdateViewport;
};

struct XpReplyUpdateViewport : public XpReplyHeader {
};
#endif

typedef union {
	XpReplyConnect a0;
	XpReplyXNBAlloc a1;
	XpReplyXNBRecv a2;
	XpReplyXNBSend a3;
	XpReplyXNBRelease a4;
	XpReplyGetIfconfig a5;
	XpReplyZutexWaitPrepare a6;
	XpReplyZutexWaitCommit a7;
	XpReplyZutexWaitAbort a8;
	XpReplyZutexWake a9;
	XpReplyLaunch a10;
	XpReplyEventPoll a11;
	XpReplySubletViewport a12;
	XpReplyRepossessViewport a13;
	XpReplyGetDeedKey a14;
	XpReplyAcceptViewport a15;
	XpReplyTransferViewport a16;
	XpReplyVerifyLabel a17;
	XpReplyMapCanvas a18;
	XpReplyUnmapCanvas a19;
	XpReplyUpdateCanvas a20;
	XpReplyReceiveUIEvent a21;
	XpReplyEndorseMe a22;
	XpReplyGetAppSecret a23;
	XpReplyDebugCreateToplevelWindow a24;
	XpReplyGetRandom a25;
} XpMaxReply;
#define MAX_REPLY_LENGTH	sizeof(XpMaxReply)

//////////////////////////////////////////////////////////////////////////////

#define XAX_PORT_RENDEZVOUS	"/tmp/xax_port_socket"
