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
| PAL_UI
|
| Purpose: Process Abstraction Layer User-Interface Functions
----------------------------------------------------------------------*/

#pragma once

#include "pal_abi/pal_types.h"

/*
A Zoog client provides first-class UI primitives because the user
interface is on the trusted path between user and each application.
Hence the UI ends up in the TCB one way or another, so the primitives
may as well be first class. (Contrast with a design that put a vendor
in charge of the display frame buffer in an attempt to keep display
abstractions out of the core Zoog kernel: that vendor's code would all
be in every app's effective TCB anyway.)

As everywhere in the Zoog Client Execution Interface, the design goal
is to provide the minimum support necessary to implement UI functionality,
wherever possible leaving all further abstractions to app-supplied
libraries.
*/

// Landlord calls

typedef void (*zf_zoog_sublet_viewport)(
	ViewportID tenant_viewport,
	ZRectangle *rectangle,
	ViewportID *out_landlord_viewport,
	Deed *out_deed);

typedef void (*zf_zoog_repossess_viewport)(
	ViewportID landlord_viewport);
	// Cancels any outstanding lease of the viewport, invalidating
	// the entire tree of viewports below, and unmapping the corresponding
	// canvases.

// How shall landlord learn of DeedKey changes?
// We could provide an async event, or we could just
// make him call down every time he wants to use the key.
// In any case, there's the possibility that his in-band message
// goes out with a stale key, in which case it may get lost.
// So we still want an async event, I guess, to relate the
// sequencing? Anyway, for now, we'll do something race-y and wrong.
typedef void (*zf_zoog_get_deed_key)(
	ViewportID landlord_viewport,
	DeedKey *out_deed_key);

// Tenant calls

typedef void (*zf_zoog_accept_viewport)(
	Deed *deed,
	ViewportID *out_tenant_viewport,
	DeedKey *out_deed_key);

typedef void (*zf_zoog_transfer_viewport)(
	ViewportID tenant_viewport,
	Deed *out_deed);
	// At the moment this call occurs, the DeedKey associated with this
	// viewport becomes invalid;
	// when the deed is accept_viewport()ed,
	// a new DeedKey is manufactured and supplied.

//////////////////////////////////////////////////////////////////////////////
//	The canvas calls have no security implication other than Tv's
//	validity, since the Tv handle was checked at "open time"
//	(accept_viewport).
//////////////////////////////////////////////////////////////////////////////

/*----------------------------------------------------------------------
| Function Pointer: zf_zoog_verify_label
|
| Purpose: Verify the legitimacy of the label associated with this principal
|          using the supplied certificate chain    
|	Until an app calls verify_label, any calls to map_canvas will fail,
|	because the monitor doesn't know how to label the display.
|	Note that verify labels the whole process, so after one call, canvases
|	can be mapped and unmapped all day.
|
| Parameters: 
|     chain       Series of increasingly specific certificates that
|                 that lead from Zoog's root key to the key that speaksfor
|                 this app.  At each step, the label on the signed object
|                 must have the parent's label chain as a suffix;
|                 i.e., .com can sign example.com, which can sign 
|                 www.example.com, but neither can sign harvard.edu
|                 Chain should be arranged in order of increasing specificity;
|                 i.e., root signs key1 signs key2 signs key3 ...
|                 signs this key                 
| Returns: Indicator as to whether verification succeeded
----------------------------------------------------------------------*/
typedef bool (*zf_zoog_verify_label)(ZCertChain* chain);
typedef enum PixelFormat {
	zoog_pixel_format_invalid = 0,
	zoog_pixel_format_truecolor24 = 1,
} PixelFormat;

// TODO does real hardware use 3 or 4 bytes per 24-bit pixel?
// The answer from X seems to be "4".
// Please tell me there aren't multiple choices.
static __inline uint8_t zoog_bytes_per_pixel(PixelFormat pixel_format)
{
	switch (pixel_format)
	{
		case zoog_pixel_format_truecolor24:
			return 4;
			// bytes: pad, R, G, B
#if 0 // possible future formats.
		case zoog_pixel_format_truecolor48:
			return 6;
			// bytes: RR GG BB
		case zoog_pixel_format_stereo24:
			return 6;
			// bytes: RGB (left), RGB (right)
		case zoog_pixel_format_stereo48:
			return 12;
			// bytes: RRGGBB (left), RRGGBB (right)
#endif
		default:
			return 0;
	}
};

typedef uint32_t ZCanvasID;
	// A host-generated handle, meaningful only inside a single app instance.

typedef struct {
	ZCanvasID canvas_id;
	PixelFormat pixel_format;
	uint32_t width;
	uint32_t height;
#if 0 // possible future extension if needed for performance, to avoid copies
	uint32_t pixels_per_line;
#else
// dear reader, please #define pixels_per_line width
#endif
	uint8_t *data;
} ZCanvas;
	// data points to a region at least
	//		len=zoog_bytes_per_pixel(pixel_format)*pixels_per_line*height
	// bytes long. That is, writes anywhere in [0..len) are guaranteed
	// not to cause a memory fault, until the canvas is closed.
	// Pixel (0,0), at the upper-left of the canvas, is at data[0].
	// Pixel (1,0), one pixel to the right of the origin,
	//		is at data[zoog_bytes_per_pixel(pixel_format)]
	// Pixel (0,1), one pixel below the origin,
	// 		is at data[zoog_bytes_per_pixel(pixel_format)*pixels_per_line]
	//
#if 0 // possible future extension if needed for performance, to avoid copies
	// NB pixels_per_line may be greater than width, for example when
	// the host provides access to a direct-mapped framebuffer. The line
	// may be 1280 pixels wide, but the ZCanvas may only cover 600 pixels,
	// the rest masked off because they're not the delegated viewport.
	// In that case, the application paints 600 pixels, then skips 680
	// pixels' worth of "other", then is on to the next line.
#endif

typedef void (*zf_zoog_map_canvas)(
	ViewportID viewport_id, PixelFormat *known_formats, int num_formats, ZCanvas *out_canvas);
	
typedef void (*zf_zoog_unmap_canvas)(ZCanvasID canvas_id);

/*----------------------------------------------------------------------
| Function Pointer: zf_zoog_update_canvas
|
| Purpose: 
|    Tells the host that the canvas memory has been updated in the
|    given rectangle.
|
|    On hosts where the canvas is to be copied to the output device,
|    the host copies the specified rectangle to the visible frame buffer.
|    
|    On hosts where the canvas is directly mapped (displayed
|    immediately), the PAL may implement this function as a no-op.
|    In particular, note there is no guarantee that updates won't
|    be propagated before an update_canvas call's rectangle
|    mentions it.
|
| Parameters:
|
| canvas_id: identifier of the canvas updated;
|    a field found in ZCanvas struct.
|
| x, y, width, height: identifies the rectangle of pixels that are
|    to be updated. Units are pixels.
|
| Returns: Nothing
----------------------------------------------------------------------*/
typedef void (*zf_zoog_update_canvas)(
	ZCanvasID canvas_id, ZRectangle *rectangle);

//////////////////////////////////////////////////////////////////////////////
// Events
//////////////////////////////////////////////////////////////////////////////


typedef enum ZoogUIEventType {
	zuie_no_event = 0,

	/* ZoogUIEvent.un is ZoogKeyEvent */
	zuie_key_down = 0x20090001,
	zuie_key_up,

	/* ZoogUIEvent.un is ZoogMouseEvent */
	zuie_mouse_move,

	/* ZoogUIEvent.un is ZoogCanvasEvent */
	zuie_focus,
	zuie_blur,
	zuie_canvas_closed,
		// the "downward" event: if a parent closes its canvas,
		// all viewports delegated from that canvas are closed,
		// and hence all the children's canvases derived from those
		// viewports are closed; the children receive these messages.
		// This message is delivered recursively, as the closure of
		// a child's canvas would close in turn any viewport delegated
		// therefrom.
	zuie_viewport_updated,	// delivered to child (delegate)

#if 0	// not yet designed
	// NB these are proposals in support of WYSIWTG privacy-sensitive device drivers
	zuie_occluded,			// window is not wholly visible
	zuie_unoccluded,		// window is wholly visible
#endif

	/* ZoogUIEvent.un is ZoogViewportEvent  */
	zuie_viewport_created,	// delivered to all potential children (delegate)
#if 0
//	zuie_viewport_closed,	// delivered to parent (delegator)
//		// the "upward" event: if a child closes its canvas, the
//		// parent's viewport is closed, generating this message.
//	Not sure I care about this message, or about the corresponding
//	downward one. This one is especially problematic, because when the
//	viewport is resized, the protocol examples below *require* the child
//	app to close and reopen the canvas (to get a new shm segment for the
//	frame buffer). There's no way for the parent to know whether a close
//	is about to be followed by an open without waiting and a timeout.
//	So this protocol needs to be better thought about.
//	One reasonable conclusion: coordination between parent and child in
//	general happens via IP protocol channels anyway, so we should leave it
//	to the apps to tell each other what's going on.
//	I'm not so sure that's a good answer, because (a) in this case, that's
//	a protocol *everyone* has to agree on, so pushing it out to IP is
//	a falacious application of minimality, and (b) it means that parents
//	have no way to detect defection by broken/malicious children, and clean
//	up. (In general, nor can the parent detect a child that has kept the
//	window open and is not drawing in it; so that's not necessarily so bad).
#endif

#if 0	// not yet designed
	zuie_drag_enter,		// delivered to dropee
	zuie_drop,				// delivered to dropee
	zuie_drop_accepted,		// delivered to dragee
	zuie_drop_canceled,		// delivered to dragee
#endif
} ZoogUIEventType;

// TODO BP suggests that some applications may want to learn the
// relative positions of their windows.

// TODO what do we supply? Keycodes? These keycodes should map to what
// the keycaps say. Now what about dvorak -- it's in the
// "accessibility"/"internationalization" bucket.
// does that happen in the Zoog client, or do we ask apps to negotiate how
// to manage it? Having key layout support vary across apps would indeed
// be unfortunate.
// NB mouse button(s) are keycodes and appear as key_down/key_up events,
// not mouse events.
typedef struct {
	ZCanvasID canvas_id;
	uint32_t keysym;
} ZoogKeyEvent;

static const uint32_t _zoog_keysym_button_base = 0x20090000;
static const int zoog_keysym_max_button = 255;
static __inline bool zoog_keysym_is_button(uint32_t keysym)
	{ return (keysym & 0xffffff00) == _zoog_keysym_button_base; }
static __inline uint32_t zoog_keysym_to_button(uint32_t keysym)
	{ return (keysym & 0x000000ff); }
static __inline uint32_t zoog_button_to_keysym(uint32_t button)
	{ return _zoog_keysym_button_base + button; }

typedef struct {
	ZCanvasID canvas_id;
	// mouse pointer position relative to ZCanvas origin
	uint32_t x;
	uint32_t y;
} ZoogMouseEvent;

// This event is received by every application matching the
// VendorKey of the viewport; apps may use the viewport ID
// to decide which app instance accepts the viewport
// with zf_zoog_accept_canvas.
typedef struct {
	ViewportID viewport_id;
} ZoogViewportEvent;

typedef struct {
	ZCanvasID canvas_id;
} ZoogCanvasEvent;

typedef struct {
	ZoogUIEventType type;
	union {
		ZoogKeyEvent key_event;
		ZoogMouseEvent mouse_event;
		ZoogCanvasEvent canvas_event;
		ZoogViewportEvent viewport_event;
	} un;
} ZoogUIEvent;

/*----------------------------------------------------------------------
| Function Pointer: zf_zoog_receive_ui_event
|
| Purpose: Fetch one pending UI event from the event queue.
|
| Remarks: Application may use it, then call free_net_buffer when
|   done. If the app wishes, it may modify the contents (eg header) and
|   call send() to send out the buffer to a new destination.
|
| This call does not block. If no UI event is available, it returns
| ZoogUIEvent.type == ZoogUIEventType.zuie_no_event.
|
| Parameters: pointer to a ZoogUIEvent structure to be populated with
| the received event.
|
| Returns: Pointer to the received network buffer
----------------------------------------------------------------------*/
typedef void (*zf_zoog_receive_ui_event)(ZoogUIEvent *out_event);



/*
An exemplar event sequence:
App P is parent, C is child. Assume P owns canvas V_P.

P: zf_zoog_launch_application(Boot_C, Key_C);
P: ViewportID viewport_id = zf_zoog_delegate_viewport(canvas_P, Key_C);
P: zf_zoog_update_viewport(viewport_id, 200, 300, 150, 150);

(Optionally, P sends C a net buffer to explain how this viewport is to
be used. The viewport_id is a system-wide identifier, not a process-local
handle, so that P & C can discuss the delegation using a common name.)
P: send_net_buffer(...message containing viewport_id...)
C: receive_net_buffer(...message containing viewport_id...)

C: zf_zoog_receive_ui_event(&evt);
	-> zuie_viewport_created; ZoogViewportEvent(viewport_id)
C: canvas_C = zf_zoog_accept_canvas(viewport_id);

// painting
C: writes in canvas_C->data
C: zf_zoog_update_canvas(canvas_C, x, y, w, h);
C: zf_zoog_update_canvas(canvas_C, x, y, w, h);
C: zf_zoog_update_canvas(canvas_C, x, y, w, h);

P: zf_zoog_update_viewport(viewport_id, 100, 100, 150, 150);
	// just a move -- no effect on C

P: zf_zoog_update_viewport(viewport_id, 100, 100, 140, 140);
// If a viewport is opened as a canvas when a zf_zoog_update_viewport()
// is called on it, the canvas is invalidated (no longer draws anywhere),
// and the owner will receive a ZoogCanvasResizeEvent.
C: zf_zoog_receive_ui_event(&evt);
	zuie_viewport_updated; ZoogViewportEvent(viewport_id)
C: zf_zoog_close_canvas(canvas_C);
C: canvas_C = zf_zoog_accept_canvas(viewport_id);
	// Note canvas_C->width == 140 now.
// NB C may instead have left the old canvas open long enough to copy
// the overlapping region into the new canvas, rather than redraw it.

(more painting)

P: zf_zoog_close_viewport(viewport_id);
C: zf_zoog_receive_ui_event(&evt);
	zuie_viewport_closed; ZoogViewportEvent(viewport_id, 140, 140)
C: zf_zoog_exit_application();
*/
