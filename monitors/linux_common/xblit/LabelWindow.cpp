#include "LabelWindow.h"
#include "Xblit.h"
#include "XblitCanvas.h"

class CtorWork : public XWork {
private:
	LabelWindow *lw;
public:
	CtorWork(LabelWindow *lw) : XWork(lw->xblit->get_sf()), lw(lw) {}
	void run() { lw->_ctor(); }
};

void LabelWindow::_ctor()
{
	Display *dsp = xblit->get_display();
	fontinfo = XLoadQueryFont(dsp, "9x15bold");
	label_string = strdup("empty");
	_create_window();
	paint();
}

LabelWindow::LabelWindow(Xblit *xblit)
	: xblit(xblit),
	  width(250),
	  height(20)
{
	CtorWork cw(this);
	xblit->do_work(&cw);
}

void LabelWindow::_create_window()
{
	int rc;
	Display *dsp = xblit->get_display();

	int screenNumber = DefaultScreen(dsp);
	white = WhitePixel(dsp, screenNumber);
	black = BlackPixel(dsp, screenNumber);

	Window root = DefaultRootWindow(dsp);

	win = XCreateSimpleWindow(dsp,
		root,
		0, 0,
		width, height,
		0,
		black, // border
		white );  // backgd

	XSetWindowAttributes xsa;
	xsa.override_redirect = true;
	rc = XChangeWindowAttributes(dsp, win, CWOverrideRedirect, &xsa);
	lite_assert(rc==1);

	gc = XCreateGC( dsp, win,
					 0,        // mask of values
					 NULL );   // array of values

	long eventMask = NoEventMask
		| ExposureMask
		;
	XSelectInput( dsp, win, eventMask );

	white = WhitePixel(dsp, screenNumber);
	black = BlackPixel(dsp, screenNumber);
	screen_colormap = DefaultColormap(dsp, screenNumber);
	rc = XAllocNamedColor(dsp, screen_colormap, "green", &green, &green);
	lite_assert(rc!=0);
	rc = XAllocNamedColor(dsp, screen_colormap, "yellow", &yellow, &yellow);
	lite_assert(rc!=0);
}

void LabelWindow::paint()
{
	Display *dsp = xblit->get_display();

	int rc;
	rc = XSetForeground(dsp, gc, green.pixel);
	lite_assert(rc==1);
	rc = XSetBackground(dsp, gc, black);
	lite_assert(rc==1);
	rc = XSetFillStyle(dsp, gc, FillSolid);
	lite_assert(rc==1);
	rc = XFillRectangle(dsp, win, gc, 0, 0, width, height);
	lite_assert(rc==1);

	rc = XSetForeground(dsp, gc, white);
	lite_assert(rc==1);
	rc = XSetBackground(dsp, gc, white);
	lite_assert(rc==1);
	rc = XFillRectangle(dsp, win, gc, 5, 5, width-10, height-10);

	rc = XSetForeground(dsp, gc, black);
	lite_assert(rc==1);
	rc = XSetBackground(dsp, gc, black);
	lite_assert(rc==1);
	rc = XSetFont(dsp, gc, fontinfo->fid);
	lite_assert(rc==1);
	rc = XDrawString(dsp, win, gc, 5, height-3,
		label_string, strlen(label_string));
	fprintf(stderr, "painted label \"%s\"\n", label_string);
	lite_assert(rc==0);

	// XSetWMProtocols(dsp, win, xblit->get_wm_delete_message_atom(), 1);
	// TODO Remove decorations

	XFlush(dsp);
}

#if 0
class ShowWork : public XWork {
private:
	LabelWindow *lw;
	XblitCanvas *parent_xbc;
public:
	ShowWork(LabelWindow *lw, XblitCanvas *parent_xbc)
		: XWork(lw->xblit->get_sf()), lw(lw), parent_xbc(parent_xbc)
	{}
	void run() { lw->_show_inner(parent_xbc); }
};

void LabelWindow::x_show(XblitCanvas *parent_xbc)
{
	ShowWork sw(this, parent_xbc);
	xblit->do_work(&sw);
}
#endif

void LabelWindow::_set_label(const char *str)
{
	free(label_string);
	label_string = strdup(str);
}

void LabelWindow::x_show(XblitCanvas *parent_xbc)
{
	ZRectangle anchor;
	parent_xbc->x_get_root_location(&anchor);
	_set_label(parent_xbc->get_window_label());

	int rc;
	Display *dsp = xblit->get_display();
	rc = XMapWindow( dsp, win );
	lite_assert(rc==1);
	rc = XMoveWindow( dsp, win, anchor.x + 32, anchor.y - height);
//	fprintf(stderr, "XMoveWindow(%d,%d)\n", anchor.x, anchor.y);
	lite_assert(rc==1);
	paint();
}

#if 0
class HideWork : public XWork {
private:
	LabelWindow *lw;
public:
	HideWork(LabelWindow *lw)
		: XWork(lw->xblit->get_sf()), lw(lw)
	{}
	void run() { lw->_hide_inner(); }
};

void LabelWindow::hide()
{
	HideWork hw(this);
	xblit->do_work(&hw);
}
#endif

void LabelWindow::x_hide()
{
	int rc;
	Display *dsp = xblit->get_display();
	rc = XUnmapWindow( dsp, win );
	lite_assert(rc==1);
	_set_label("none");
}

