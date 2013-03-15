#include <poll.h>
#include <unistd.h>
#include <asm/unistd.h>

#include "Xblit.h"
#include "LiteLib.h"
#include "standard_malloc_factory.h"
#include "XBlitCanvasAcceptorIfc.h"
#include "LabelWindow.h"

//////////////////////////////////////////////////////////////////////////////

class WindowCanvasMapping {
public:
	WindowCanvasMapping(Window w, XblitCanvas *c) : window(w), canvas(c) {}
	Window window;
	XblitCanvas *canvas;
	static uint32_t hash(const void *v_wcm);
	static int cmp(const void *v_a, const void *v_b);
};

uint32_t WindowCanvasMapping::hash(const void *v_wcm)
{
	WindowCanvasMapping *wcm = (WindowCanvasMapping *) v_wcm;
	return (uint32_t) wcm->window;
}

int WindowCanvasMapping::cmp(const void *v_a, const void *v_b)
{
	WindowCanvasMapping *wcm_a = (WindowCanvasMapping *) v_a;
	WindowCanvasMapping *wcm_b = (WindowCanvasMapping *) v_b;
	return ((uint32_t) wcm_a->window) - ((uint32_t) wcm_b->window);
}

//////////////////////////////////////////////////////////////////////////////

Display *Xblit::_open_display()
{
	Display *_dsp = XOpenDisplay(NULL);
	lite_assert(_dsp!=NULL);

	if (false)
	{
		XSynchronize(_dsp, true);
		fprintf(stderr, "**** PERF WARNING: running X synchronously\n");
	}

	return _dsp;
}

Xblit::Xblit(ThreadFactory *tf, SyncFactory *sf)
	: mf(standard_malloc_factory_init()),
	  sf(sf),
	  x_mutex(sf->new_mutex(true)),
	  dsp(_open_display()),
	  xwork_mutex(sf->new_mutex(true)),
	  focused_canvas(NULL)
{
	linked_list_init(&xwork_queue, mf);
	int rc = pipe(xwork_pipes);
	lite_assert(rc==0);

	hash_table_init(&window_to_xc, mf, &WindowCanvasMapping::hash, &WindowCanvasMapping::cmp);

	assert(dsp!=NULL);

	wmDeleteMessage = XInternAtom(dsp, "WM_DELETE_WINDOW", False);

	tf->create_thread(_x_worker_s, this, 16<<10);

	// can't invoke label_window until we've got the event loop running.
	this->label_window = new LabelWindow(this);
}

BlitProviderCanvasIfc *Xblit::create_canvas(
	ZCanvasID canvas_id,
	ZRectangle *wr,
	BlitCanvasAcceptorIfc *canvas_acceptor,
	ProviderEventDeliveryIfc *event_delivery_ifc,
	BlitProviderCanvasIfc *parent,
	const char *window_label)
{
	XBlitCanvasAcceptorIfc *xacceptor =
		(XBlitCanvasAcceptorIfc *) canvas_acceptor;

	uint32_t framebuffer_size = wr->width * wr->height
		* zoog_bytes_per_pixel(zoog_pixel_format_truecolor24);
	ZCanvas *zoog_canvas = xacceptor->make_canvas(framebuffer_size);
	if (zoog_canvas==NULL)
	{
		lite_assert(false); // unhandled pool-full case. see transaction.cpp:XNBAlloc
	}
	zoog_canvas->canvas_id = canvas_id;
	zoog_canvas->data = (uint8_t*) 0x00000111;
		// this is the pal-relative address. We void it here to remind
		// us that PAL should fill it in.

	XblitCanvas *parent_xbc = static_cast<XblitCanvas *>(parent);
	XblitCanvas *xbc = new XblitCanvas(
		this, canvas_id, wr, xacceptor, event_delivery_ifc, parent_xbc, zoog_canvas);
	xbc->set_window_label(window_label);

	return xbc;
}

Xblit::~Xblit()
{
	// todo close all canvasii
	x_mutex->lock();
	XCloseDisplay(dsp);
	x_mutex->unlock();
}

void Xblit::_x_worker_s(void *v_this)
{
	((Xblit*)v_this)->_x_worker();
}

void Xblit::_update_zoog_focus(Window win)
{
	if (win==zoog_focused_window)
	{
		return;	// already there
	}
	
	fprintf(stderr, "_update_zoog_focus %x\n", (int) win);
	XblitCanvas *bc = lookup_canvas_by_window(win);
	if (bc==NULL)
	{
		if (focused_canvas != NULL)
		{
			label_window->x_hide();
			focused_canvas = NULL;
		}
	}
	else
	{
		if (focused_canvas != bc)
		{
			label_window->x_show(bc);
			focused_canvas = bc;
		}
	}
	zoog_focused_window = win;
}

void Xblit::_focus_in(XFocusChangeEvent *evt)
{
//	fprintf(stderr, "focus_in %x\n", (int) evt->window);
	_update_zoog_focus(evt->window);
#if 0
	XblitCanvas *bc = lookup_canvas_by_window(evt->window);
	if (bc==NULL)
	{
		return;
	}
	if (focused_canvas != bc)
	{
		label_window->x_show(bc);
		focused_canvas = bc;
	}
#endif
}

void Xblit::_focus_out(XFocusChangeEvent *evt)
{
	_update_zoog_focus((Window) 0);
#if 0
	XblitCanvas *bc = lookup_canvas_by_window(evt->window);
	if (bc==NULL)
	{
		return;
	}
	if (focused_canvas == bc)
	{
		label_window->x_hide();
		focused_canvas = NULL;
	}
#endif
}

void Xblit::do_work(XWork *xwork)
{
	if (_gettid()==_x_tid)
	{
		xwork->run();
	}
	else
	{
		xwork_mutex->lock();
		linked_list_insert_head(&xwork_queue, xwork);
		char x=' ';
		write(xwork_pipes[1], &x, 1);
		xwork_mutex->unlock();
		xwork->event->wait();
		xwork->destruct();
	}
}

int Xblit::_gettid()
{
	return syscall(__NR_gettid);
}

void Xblit::_x_worker()
{
	XEvent evt;

	_x_tid = _gettid();

	x_mutex->lock();
	int x_fd = ConnectionNumber(dsp);
	x_mutex->unlock();

	while (true)
	{
		enum { X_FD=0, PIPE_FD=1 };
		struct pollfd pfd[2];
		pfd[X_FD].fd = x_fd;
		pfd[X_FD].events = POLLIN;
		pfd[X_FD].revents = 0;
		pfd[PIPE_FD].fd = xwork_pipes[0];
		pfd[PIPE_FD].events = POLLIN;
		pfd[PIPE_FD].revents = 0;
		int nfds = 2;

//		fprintf(stderr, "poll starts.\n");
		int poll_rc = poll(pfd, nfds, -1);
//		fprintf(stderr, "poll completes; %d\n", pfd[X_FD].revents);

		if (pfd[PIPE_FD].revents != 0)
		{
//			fprintf(stderr, "scanning XWork queue\n");
			xwork_mutex->lock();
			while (xwork_queue.count>0)
			{
//				fprintf(stderr, "absorb pipe char\n");
				char dummy;
				read(pfd[PIPE_FD].fd, &dummy, 1);

//				fprintf(stderr, "remove first of %d\n", xwork_queue.count);
				XWork *xwork = (XWork*) linked_list_remove_head(&xwork_queue);
				xwork->run();
				xwork->event->signal();
			}
			xwork_mutex->unlock();
		}

		lite_assert(poll_rc>0);

		if (pfd[X_FD].revents != 0)
		{
//			fprintf(stderr, "scanning X events\n");
			while (true)
			{
				x_mutex->lock();
				int pending = XPending(dsp);
//				fprintf(stderr, "XPending()==%d\n", pending);
//				fprintf(stderr, "XNextEvent(\n");
				if (pending>0)
				{
					XNextEvent(dsp, &evt);
				}
//				fprintf(stderr, ")\n");
				x_mutex->unlock();

				if (pending==0)
				{
					break;
				}

				switch (evt.type)
				{
					case FocusIn:
						_focus_in((XFocusChangeEvent*) &evt);
						break;
					case FocusOut:
						_focus_out((XFocusChangeEvent*) &evt);
						break;
					case MapNotify:
						_map_notify((XMappingEvent*) &evt);
						break;
					case MotionNotify:
						_mouse_move((XMotionEvent*) &evt);
						break;
					case KeyPress:
						_key_event((XKeyEvent *) &evt, zuie_key_down);
						break;
					case KeyRelease:
						_key_event((XKeyEvent *) &evt, zuie_key_up);
						break;
					case ButtonPress:
						_mouse_button_event((XButtonEvent *) &evt, zuie_key_down);
						break;
					case ButtonRelease:
						_mouse_button_event((XButtonEvent *) &evt, zuie_key_up);
						break;
					case Expose:
						_expose((XExposeEvent *) &evt);
						break;
					case ClientMessage:
						_client_message_event((XClientMessageEvent *) &evt);
						break;
					default:
						fprintf(stderr, "unknown evt.type==%d\n", evt.type);
						break;
				}
			}
		}
	}
}

void Xblit::_map_notify(XMappingEvent *me)
{
	fprintf(stderr, "_map_notify: X window %08x mapped\n", (uint32_t) me->window);
	XblitCanvas *bc = lookup_canvas_by_window(me->window);
	if (bc != NULL)
	{
		bc->window_mapped();
	}
	else
	{
		lite_assert(false);	// lookup failed!?
	}
}

void Xblit::_mouse_move(XMotionEvent *me)
{
//	fprintf(stderr, "MotionNotify (%d,%d)\n", me->x, me->y);
	// TODO should generally clear the empty space when delivering
	// these events
	XblitCanvas *bc = lookup_canvas_by_window(me->window);
	if (bc!=NULL)
	{
		ZoogUIEvent zuie;
		zuie.type = zuie_mouse_move;
		zuie.un.mouse_event.canvas_id = bc->get_canvas_id();
		zuie.un.mouse_event.x = me->x;
		zuie.un.mouse_event.y = me->y;
		// need to correct these to subwindows, unless we
		// actually just use X child windows, which in fact
		// is what we should do.
		bc->deliver_ui_event(&zuie);
	}
}

void Xblit::_key_event(XKeyEvent *xke, ZoogUIEventType which)
{
//	fprintf(stderr, "keystroke in window %x\n", (int) xke->window);
	_update_zoog_focus(xke->window);
	XblitCanvas *bc = lookup_canvas_by_window(xke->window);
	// TODO assert that we're only ever delivering keystrokes to
	// a focused canvas.
	if (bc!=NULL)
	{
		ZoogUIEvent zuie;
		zuie.type = which;
		zuie.un.key_event.canvas_id = bc->get_canvas_id();

		KeySym ks;
		char dummy[256];
		XLookupString(xke, dummy, sizeof(dummy), &ks, NULL);

		zuie.un.key_event.keysym = ks;

		bc->deliver_ui_event(&zuie);
	}
}

void Xblit::_mouse_button_event(XButtonEvent *xbe, ZoogUIEventType which)
{
//	fprintf(stderr, "click in window %x\n", (int) xbe->window);
	_update_zoog_focus(xbe->window);
	XblitCanvas *bc = lookup_canvas_by_window(xbe->window);
	if (bc!=NULL)
	{
		ZoogUIEvent zuie;
		zuie.type = which;
		zuie.un.key_event.canvas_id = bc->get_canvas_id();
		int button = xbe->button - 1;
		lite_assert(button >= 0);
		lite_assert(button <= zoog_keysym_max_button);
		zuie.un.key_event.keysym = zoog_button_to_keysym(button);
		bc->deliver_ui_event(&zuie);
	}
}

void Xblit::_expose(XExposeEvent *xee)
{
	XblitCanvas *bc = lookup_canvas_by_window(xee->window);
	if (bc!=NULL)
	{
		ZRectangle wr;
		wr.x = xee->x;
		wr.y = xee->y;
		wr.width = xee->width;
		wr.height = xee->height;
		bc->x_update_image(&wr);
	}
	else if (xee->window==label_window->get_window())
	{
		label_window->paint();
	}
}

void Xblit::_client_message_event(XClientMessageEvent *xcme)
{
	if (xcme->data.l[0] != (int) wmDeleteMessage)
	{
		fprintf(stderr, "Unknown XClientMessageEvent %ld; dropping\n",
			xcme->data.l[0]);
		return;
	}

	XblitCanvas *bc = lookup_canvas_by_window(xcme->window);
	delete bc;
}

void Xblit::deregister_canvas(XblitCanvas *bc, Window window)
{
	WindowCanvasMapping wck(window, NULL);
	WindowCanvasMapping *wcm = (WindowCanvasMapping *)
		hash_table_lookup(&window_to_xc, &wck);
	wcm->canvas = NULL;
	// leave hash table entry in place, but mark window ID dead.
}

void Xblit::register_canvas(XblitCanvas *bc, Window window)
{
	WindowCanvasMapping *wcm = new WindowCanvasMapping(window, bc);
	hash_table_insert(&window_to_xc, wcm);
}

XblitCanvas *Xblit::lookup_canvas_by_window(Window window)
{
	WindowCanvasMapping wck(window, NULL);
	WindowCanvasMapping *wcm = (WindowCanvasMapping *)
		hash_table_lookup(&window_to_xc, &wck);
	if (wcm!=NULL)
	{
		return wcm->canvas;
	}
	else
	{
		return NULL;
	}
}
