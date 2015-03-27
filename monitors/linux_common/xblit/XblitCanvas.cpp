#include "XblitCanvas.h"
#include "Xblit.h"
#include "LiteLib.h"

XblitCanvas::XblitCanvas(
	Xblit *xblit,
	ZCanvasID canvas_id,
	ZRectangle *wr,
	XBlitCanvasAcceptorIfc *xacceptor,
	ProviderEventDeliveryIfc *event_delivery_ifc,
	XblitCanvas *parent_xbc,
	ZCanvas *zoog_canvas)
{
	this->backing_image = NULL;	// be sure premature incoming events can be safely squelched
	this->xblit = xblit;
	this->canvas_id = canvas_id;
	this->event_delivery_ifc = event_delivery_ifc;
	window_mapped_event = xblit->get_sync_factory()->new_event(false);
	this->_window_label = strdup("unlabeled--bug");
		// window should get a label before it gets focused

	_create_window(wr, parent_xbc);
	// here we don't have the X lock, so the event thread can deliver our
	// MapNotify.
	window_mapped_event->wait();
	_set_up_image(wr->width, wr->height, xacceptor->get_framebuffer(), xacceptor->get_framebuffer_size());

	define_zoog_canvas(zoog_canvas);
}

class DestroyWork : public XWork {
private:
	XblitCanvas *xbc;
public:
	DestroyWork(XblitCanvas *xbc) : XWork(xbc->xblit->get_sf()), xbc(xbc) {}
	void run() { XDestroyWindow(xbc->xblit->get_display(), xbc->win); }
};

XblitCanvas::~XblitCanvas()
{
	{
		ZoogUIEvent zuie;
		zuie.type = zuie_canvas_closed;
		zuie.un.canvas_event.canvas_id = get_canvas_id();
		deliver_ui_event(&zuie);
	}	

	// tell zoog apps this canvas is gone
	event_delivery_ifc->notify_canvas_closed(this);

	// tell provider not to try to deliver any events in through
	// this window
	xblit->deregister_canvas(this, win);

	// and finally, tear it down.
	DestroyWork destroy_work(this);
	xblit->do_work(&destroy_work);
}

void XblitCanvas::window_mapped()
{
	window_mapped_event->signal();
}

void XblitCanvas::deliver_ui_event(ZoogUIEvent *zuie)
{
	event_delivery_ifc->deliver_ui_event(zuie);
}

class CreateWork : public XWork {
private:
	XblitCanvas *xbc;
	ZRectangle *wr;
	XblitCanvas *parent_xbc;
public:
	CreateWork(XblitCanvas *xbc, ZRectangle *wr, XblitCanvas *parent_xbc)
		: XWork(xbc->xblit->get_sf()), xbc(xbc), wr(wr), parent_xbc(parent_xbc)
		{ }
	void run() { xbc->_create_window_inner(wr, parent_xbc); }
};

void XblitCanvas::_create_window(ZRectangle *wr, XblitCanvas *parent_xbc)
{
	CreateWork cw(this, wr, parent_xbc);
	xblit->do_work(&cw);
}

void XblitCanvas::_create_window_inner(ZRectangle *wr, XblitCanvas *parent_xbc)
{
	Display *dsp = xblit->get_display();
#if 0
	// we never actually got shm working
	// and on inoe, XShmPixmapFormat returned XYPixmap! (1-bit depth!)
	Status rc = XShmQueryExtension(dsp);
	assert(rc);

	int rci = XShmPixmapFormat(dsp);
	assert(rci==ZPixmap);
#endif

	int screenNumber = DefaultScreen(dsp);
	unsigned long white = WhitePixel(dsp,screenNumber);
	unsigned long black = BlackPixel(dsp,screenNumber);

	Window parent_window;
	if (parent_xbc!=NULL)
	{
		parent_window = parent_xbc->win;
		// won't be getting a mapped event...
		window_mapped_event->signal();
	} else {
		parent_window = DefaultRootWindow(dsp);
	}

	win = XCreateSimpleWindow(dsp,
		parent_window,
		wr->x, wr->y,   // origin
		wr->width, wr->height, // size
		0, black, // border
		white );  // backgd

	// need to register this right away, so that our MapNotify event can
	// be routed hereward.
	xblit->register_canvas(this, win);

	XMapWindow( dsp, win );

	long eventMask = NoEventMask
		| StructureNotifyMask
		| KeyPressMask
		| KeyReleaseMask
		| ButtonPressMask
		| ButtonReleaseMask
		| PointerMotionMask
		| ExposureMask
		| FocusChangeMask
		;
	XSelectInput( dsp, win, eventMask );

	XSetWMProtocols(dsp, win, xblit->get_wm_delete_message_atom(), 1);

	XFlush(dsp);
}

class SetupWork : public XWork {
private:
	XblitCanvas *xbc;
	uint32_t width; uint32_t height; uint8_t *data; uint32_t capacity;
public:
	SetupWork(XblitCanvas *xbc, uint32_t width, uint32_t height, uint8_t *data, uint32_t capacity)
		: XWork(xbc->xblit->get_sf()), xbc(xbc), width(width), height(height), data(data), capacity(capacity)
		{ }
	void run() { xbc->_set_up_image_inner(width, height, data, capacity); }
};

void XblitCanvas::_set_up_image(uint32_t width, uint32_t height, uint8_t *data, uint32_t capacity)
{
	SetupWork sw(this, width, height, data, capacity);
	xblit->do_work(&sw);
}

void XblitCanvas::_set_up_image_inner(uint32_t width, uint32_t height, uint8_t *data, uint32_t capacity)
{
	Display *dsp = xblit->get_display();
	gc = XCreateGC( dsp, win,
					 0,        // mask of values
					 NULL );   // array of values

	int screenNumber = DefaultScreen(dsp);
	unsigned long black = BlackPixel(dsp,screenNumber);
	XSetForeground( dsp, gc, black );

	int depth = 24;

	int num = 0;
	XVisualInfo visual_template;
	visual_template.depth = depth;
	XVisualInfo *vinfos = XGetVisualInfo(dsp, VisualDepthMask, &visual_template, &num);
	assert(num>0);
	Visual *visual = vinfos[0].visual;
	(void) visual;
	
	int bytes_per_pixel = 4;
	assert(width*bytes_per_pixel*height <= capacity);
	int offset = 0;
	int bitmap_pad = 32;
	backing_image = XCreateImage(dsp, visual, depth, ZPixmap, offset,
		(char*) data, width, height, bitmap_pad, width*bytes_per_pixel);
#if 0
	uint32_t plane_mask = 0xffffffff;	// TODO must be a symbol for this in X hdrs somewhere
	backing_image = XGetImage(dsp, win, 0, 0, 200, 200, plane_mask, ZPixmap);
#endif
}

void XblitCanvas::define_zoog_canvas(ZCanvas *canvas)
{
	int bytes_per_pixel = (backing_image->bits_per_pixel)>>3;
	assert(bytes_per_pixel == zoog_bytes_per_pixel(zoog_pixel_format_truecolor24));
	assert(backing_image->width*bytes_per_pixel
		== backing_image->bytes_per_line);
	canvas->canvas_id = canvas_id;
	canvas->pixel_format = zoog_pixel_format_truecolor24;
	canvas->width = backing_image->width;
	canvas->height = backing_image->height;
	canvas->data = (uint8_t*) backing_image->data;
}

class UpdateWork : public XWork {
private:
	XblitCanvas *xbc;
	ZRectangle *wr;
public:
	UpdateWork(XblitCanvas *xbc, ZRectangle *wr)
		: XWork(xbc->xblit->get_sf()), xbc(xbc), wr(wr)
		{ }
	void run() { xbc->x_update_image(wr); }
};

void XblitCanvas::update_image(ZRectangle *wr)
{
	UpdateWork uw(this, wr);
	xblit->do_work(&uw);
}

void XblitCanvas::x_update_image(ZRectangle *wr)
{
	if (backing_image==NULL)
	{
		fprintf(stderr, "dropping early notification; backing_image not ready\n");
		return;
	}

	fprintf(stderr, "update_image(%d, %d, %d, %d), first pixel %08x\n",
		wr->x, wr->y, wr->width, wr->height, ((uint32_t*)backing_image->data)[wr->x+wr->y*backing_image->width]);
	Display *dsp = xblit->get_display();
	int rci = XPutImage(dsp, win, gc, backing_image, wr->x, wr->y, wr->x, wr->y, wr->width, wr->height);
	lite_assert(rci==0);
	rci = XFlush(dsp);
	//lite_assert(rci==0); // undocumented, but definitely not zero!
}

class SetWindowLabelWork : public XWork {
private:
	XblitCanvas *xbc;
	const char* window_label;
public:
	SetWindowLabelWork(XblitCanvas *xbc, const char* window_label)
		: XWork(xbc->xblit->get_sf()), xbc(xbc), window_label(window_label)
		{ }
	void run() { xbc->x_set_window_label(window_label); }
};

void XblitCanvas::set_window_label(const char *window_label)
{
	SetWindowLabelWork swlw(this, window_label);
	xblit->do_work(&swlw);
}

void XblitCanvas::x_set_window_label(const char *window_label)
{
	if (_window_label!=NULL)
	{
		free(_window_label);
	}
#if 1
	char buf[800];
	snprintf(buf, sizeof(buf), "%s", window_label);
	_window_label = strdup(buf);
#else
	_window_label = strdup(window_label);
#endif
	Display *dsp = xblit->get_display();
	XStoreName(dsp, win, window_label);
}

void XblitCanvas::reconfigure(ZRectangle *wr)
{
	Display *dsp = xblit->get_display();
	int rc = XMoveResizeWindow(dsp, win, wr->x, wr->y, wr->width, wr->height);
	lite_assert(rc==0);
}

void XblitCanvas::x_get_root_location(ZRectangle *out_rect)
{
	Display *dsp = xblit->get_display();
	Window root;
	int rel_x, rel_y;
	unsigned border_width;
	unsigned depth;
	Status st = XGetGeometry(dsp, win, &root,
		&rel_x, &rel_y,
		&out_rect->width, &out_rect->height,
		&border_width, &depth);
	lite_assert(st==1);

	Window child;
	st = XTranslateCoordinates(dsp, win, root, 0, 0,
		(int*) &out_rect->x, (int*) &out_rect->y, &child);
	fprintf(stderr, "root %d,%d; translated %d,%d\n",
		rel_x, rel_y,
		out_rect->x, out_rect->y);
	lite_assert(st==1);
}

char *XblitCanvas::get_window_label()
{
	return _window_label;
}
