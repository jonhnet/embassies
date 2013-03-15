#include <stdint.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>

int main()
{
	Display *dsp = XOpenDisplay( NULL );
	assert(dsp!=NULL);

	Status rc = XShmQueryExtension(dsp);
	assert(rc);

	int rci = XShmPixmapFormat(dsp);
	assert(rci==ZPixmap);

	int screenNumber = DefaultScreen(dsp);
	unsigned long white = WhitePixel(dsp,screenNumber);
	unsigned long black = BlackPixel(dsp,screenNumber);

	Window win = XCreateSimpleWindow(dsp,
		DefaultRootWindow(dsp),
		50, 50,   // origin
		200, 200, // size
		0, black, // border
		white );  // backgd

	XMapWindow( dsp, win );

	long eventMask = StructureNotifyMask;
	XSelectInput( dsp, win, eventMask );

	XEvent evt;
	do{
	XNextEvent( dsp, &evt );   // calls XFlush
	}while( evt.type != MapNotify );


	GC gc = XCreateGC( dsp, win,
					 0,        // mask of values
					 NULL );   // array of values
	XSetForeground( dsp, gc, black );

	XDrawLine(dsp, win, gc, 10, 10,190,190); //from-to
	XDrawLine(dsp, win, gc, 10,190,190, 10); //from-to

	int depth = 24;

	int num = 0;
	XVisualInfo template;
	template.depth = depth;
	XVisualInfo *vinfos = XGetVisualInfo(dsp, VisualDepthMask, &template, &num);
	assert(num>0);
	Visual *visual = vinfos[0].visual;
	(void) visual;
	fprintf(stderr, "Found %d matching visuals\n", num);

/*
	XShmSegmentInfo shminfo;
	XImage *ximage = XShmCreateImage(dsp,
		visual,
		depth, ZPixmap, NULL,
		&shminfo, 160, 160);
	assert(ximage != NULL);
	shminfo.shmid = shmget(IPC_PRIVATE,
		ximage->bytes_per_line * ximage->height,
		IPC_CREAT|0777);
	assert(shminfo.shmid != -1);
	ximage->data = shmat(shminfo.shmid, NULL, 0);
	assert(ximage->data != (void*) -1);
	shminfo.shmaddr = ximage->data;
	shminfo.readOnly = False;
	rc = XShmAttach(dsp, &shminfo);
	assert(rc);

	rc = XShmPutImage(dsp, win, gc, ximage, 0, 0, 20, 20, 160, 160, False);
	assert(rc);
*/

	int width = 160;
	int height = 160;
	int bytes_per_pixel = 4;
	char *data = malloc(width*height*bytes_per_pixel);
	uint32_t i;
	int bitmap_pad = 32;
	XImage *ximage;
	int format = ZPixmap;
	for (i=0; i<num && i<5; i++)
	{
		Visual *sub_visual = vinfos[i].visual;
		ximage = XCreateImage(dsp, sub_visual, depth, format,
			0, // offset
			data, width, height, bitmap_pad, width*bytes_per_pixel);
		fprintf(stderr, "XCreateImage result at [%d], visual id %x: %d\n",
			i,
			(uint32_t) vinfos[i].visualid,
			ximage!=NULL);
	}
	//assert(ximage!=NULL);

	uint32_t plane_mask = 0xffffffff;	// TODO must be a symbol for this in X hdrs somewhere
	XImage *getimage = XGetImage(dsp, win, 20, 20, 160, 160, plane_mask, ZPixmap);
	(void) getimage;

	{
		int x, y;
		int bytes_per_pixel = (getimage->bits_per_pixel)>>3;
		for (y=0; y<getimage->height; y++)
		{
			for (x=0; x<getimage->width; x++)
			{
				if ((x&0xf)<5)
				{
					uint32_t *pixel_addr = (uint32_t*)
						(getimage->data + getimage->bytes_per_line*y + bytes_per_pixel*x);
					pixel_addr[0] = 0x00ff8800;
				}
			}
		}
	}

//	rci = XPutImage(dsp, win, gc, ximage, 0, 0, 20, 20, 160, 160);

	rci = XPutImage(dsp, win, gc, getimage, 0, 0, 20, 20, 160, 160);
	fprintf(stderr, "XPutImage said %d\n", rc);
//	assert(rc);

//	fprintf(stderr, "ximage=%08x\n", (uint32_t) ximage);
/*
	XShmSegmentInfo shminfo;
	memset(&shminfo, 6, sizeof(shminfo));
	Pixmap pixmap = XShmCreatePixmap(dsp, win, NULL, &shminfo, 160, 160, depth);
	(void) pixmap;
*/

	// set up to catch window-close events from window manager
	Atom wmDeleteMessage = XInternAtom(dsp, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(dsp, win, &wmDeleteMessage, 1);

	eventMask = ButtonPressMask|ButtonReleaseMask;
	eventMask = 0x01ffffff;
	XSelectInput(dsp,win,eventMask); // override prev

	do{
		XNextEvent( dsp, &evt );   // calls XFlush()
		if (evt.type==ClientMessage
			&& evt.xclient.data.l[0] == wmDeleteMessage)
		{
			break;
		}
	} while( evt.type != ButtonRelease );

	XDestroyWindow( dsp, win );
	XCloseDisplay( dsp );

	return 0;
}

