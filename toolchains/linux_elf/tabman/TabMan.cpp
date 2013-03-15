#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#include "standard_malloc_factory.h"
#include "TabMan.h"
#include "xax_extensions.h"
#include "ambient_malloc.h"

static gboolean delete_event( GtkWidget *widget, GdkEvent  *event, gpointer   data )
{
    /* If you return FALSE in the "delete-event" signal handler,
     * GTK will emit the "destroy" signal. Returning TRUE means
     * you don't want the window to be destroyed.
     * This is useful for popping up 'are you sure you want to quit?'
     * type dialogs. */

    g_print ("delete event occurred\n");

    /* Change TRUE to FALSE and the main window will be destroyed with
     * a "delete-event". */

    return TRUE;
}

/* Another callback */
static void destroy( GtkWidget *widget,
                     gpointer   data )
{
    gtk_main_quit ();
}

TabMan::TabMan()
	: next_color(0),
	  last_page(NULL)
{
	zdt = xe_get_dispatch_table();

	_set_up_navigation_protocol_socket();

    /* create a new window */
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    
    /* When the window is given the "delete-event" signal (this is given
     * by the window manager, usually by the "close" option, or on the
     * titlebar), we ask it to call the delete_event () function
     * as defined above. The data passed to the callback
     * function is NULL and is ignored in the callback function. */
    g_signal_connect (window, "delete-event",
		      G_CALLBACK (delete_event), NULL);
    
    /* Here we connect the "destroy" event to a signal handler.  
     * This event occurs when we call gtk_widget_destroy() on the window,
     * or if we return FALSE in the "delete-event" callback. */
    g_signal_connect (window, "destroy",
		      G_CALLBACK (destroy), NULL);
    
    /* Sets the border width of the window. */
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);

	GtkWidget *box1 = gtk_hbox_new(FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window), box1);
    gtk_widget_show(box1);
    
	notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);

	g_signal_connect (G_OBJECT(notebook), "switch-page",
		  G_CALLBACK (switch_page_cb), (gpointer) this);

	gtk_box_pack_start (GTK_BOX(box1), notebook, FALSE, FALSE, 0);
    gtk_widget_show(notebook);

	GtkWidget *new_tab_button = gtk_button_new_with_label("New");
	g_signal_connect (G_OBJECT(new_tab_button), "clicked",
		  G_CALLBACK (new_tab_cb), (gpointer) this);
	gtk_box_pack_start(GTK_BOX(box1), new_tab_button, FALSE, FALSE, 0);
	gtk_widget_show(new_tab_button);

	linked_list_init(&pages, standard_malloc_factory_init());

	new_tab();

	gtk_widget_add_events(GTK_WIDGET(window), GDK_MAP);
//	g_signal_connect (window, "map-event",
//		  G_CALLBACK (_window_mapped_cb), (gpointer) this);

    gtk_widget_show (window);

    gtk_main();
}

void TabMan::new_tab_cb(GtkWidget *widget, gpointer data)
{
	((TabMan*) data)->new_tab();
}

void TabMan::switch_page_cb(GtkNotebook *notebook, gpointer ptr, guint page_num, gpointer data)
{
	((TabMan*) data)->switch_page(notebook, page_num);
}

void TabMan::switch_page(GtkNotebook *notebook, guint page_num)
{
	GtkWidget *page_frame = gtk_notebook_get_nth_page(notebook, page_num);
	Page *page;
	g_object_get(page_frame, "user-data", &page, NULL);
	fprintf(stderr, "Notebook switches to page %d (Page=%p)\n",
		page_num, page);
	if (last_page!=NULL)
	{
		last_page->unmap_pane();
	}
	page->map_pane();
}

void TabMan::new_tab()
{
	next_color = (next_color + 1) % 6;
	Page *page = new Page(zdt, notebook, next_color);
	linked_list_insert_tail(&pages, page);
}

void TabMan::_set_up_navigation_protocol_socket()
{
	nav_socket = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(NavigationProtocol::PORT);
	addr.sin_addr.s_addr = INADDR_ANY;
	int rc = bind(nav_socket, (struct sockaddr*) &addr, sizeof(addr));
	lite_assert(rc==0);

#if 0
	// Nonblocking lets us avoid having to integrate with poll;
	// we just loop until socket is dry.
	int flags = fcntl(nav_socket, F_GETFL, 0);
	rc = fcntl(nav_socket, F_SETFL, flags | O_NONBLOCK);
	lite_assert(rc==0);

	// Enable outgoing broadcasts. (LWIP probably just ignores this,
	// but we're trying to be a good citizen here.)
	int one = 1;
	rc = setsockopt(nav_socket, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one));
	lite_assert(rc==0 || errno==ENOSYS);
#endif

#if 0	// only available in a later gdk version...
	GIOChannel *channel = g_io_channel_unix_new(nav_socket);
	gtk_io_add_watch(channel, G_IO_IN, _read_navigation_protocol_socket_cb, this);
#endif
	gdk_input_add(nav_socket, GDK_INPUT_READ, _read_navigation_protocol_socket_cb, this);
}

void TabMan::_read_navigation_protocol_socket_cb(gpointer data, gint source, GdkInputCondition condition)
{
	((TabMan*) data)->_read_navigation_protocol_socket();
}

void TabMan::_read_navigation_protocol_socket()
{
	uint8_t msg_buf[4096];
	int rc = read(nav_socket, msg_buf, sizeof(msg_buf));
	NavigationBlob *msg = new NavigationBlob(msg_buf, rc);
	uint32_t opcode;
	msg->read(&opcode, sizeof(opcode));
	switch (opcode)
	{
	case TMTabTransferStart::OPCODE:
		_tab_transfer_start(msg);
		break;
	case TMTabTransferComplete::OPCODE:
		_tab_transfer_complete(msg);
		break;
	default:
		// that message must not have been for me.
		break;
	}
	delete msg;
}

void TabMan::_tab_transfer_start(NavigationBlob *msg)
{
	fprintf(stderr, "Received TMTabTransferStart\n");
	TMTabTransferStart tmtts;
	msg->read(&tmtts, sizeof(tmtts));
	Page *page = _map_tab_key_to_page(tmtts.tab_deed_key);
	page->tab_transfer_start();
}

Page* TabMan::_map_tab_key_to_page(DeedKey key)
{
	LinkedListIterator lli;
	for (ll_start(&pages, &lli); ll_has_more(&lli); ll_advance(&lli))
	{
		Page* page = (Page*) ll_read(&lli);
		if (page->has_tab_key(key))
		{
			return page;
		}
	}
	return NULL;
}

void TabMan::_refresh_all_page_tab_deed_keys()
{
	LinkedListIterator lli;
	for (ll_start(&pages, &lli); ll_has_more(&lli); ll_advance(&lli))
	{
		Page* page = (Page*) ll_read(&lli);
		page->refresh_tab_key();
	}
}

void TabMan::_tab_transfer_complete(NavigationBlob *msg)
{
	fprintf(stderr, "Received TMTabTransferComplete\n");
	TMTabTransferComplete tmttc;
	msg->read(&tmttc, sizeof(tmttc));
	Page *page = _map_tab_key_to_page(tmttc.tab_deed_key);
	if (page==NULL)
	{
		_refresh_all_page_tab_deed_keys();
		page = _map_tab_key_to_page(tmttc.tab_deed_key);
		if (page==NULL)
		{
			fprintf(stderr, "That's weird, I can't find the tab for the guy who sent this TransferComplete message.\n");
			lite_assert(false);
			return;
		}
	}
	page->tab_transfer_complete();
}

int main( int   argc,
          char *argv[] )
{
	ambient_malloc_init(standard_malloc_factory_init());

    gtk_init (&argc, &argv);

	TabMan tabman;
    
    return 0;
}
