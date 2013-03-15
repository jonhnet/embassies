#include <gtk/gtk.h>
#include <string.h>

#include "LiteLib.h"
#include "Page.h"

const char * colornames[] = { "red", "orange", "yellow", "green", "blue", "violet" };

Page::Page(ZoogDispatchTable_v1 *zdt, GtkWidget *notebook, uint32_t coloridx)
	: zdt(zdt),
	  coloridx(coloridx),
	  notebook(notebook)
{
	tab_box = new TabBox(this);
	pane_box = new PaneBox(this, tab_box);
	tab_box->set_pane_box(pane_box);

	lite_assert(coloridx < sizeof(colornames)/sizeof(colornames[0]));

	char bufferf[500]; sprintf(bufferf, "My Frame %d", coloridx);
	char bufferl[500]; sprintf(bufferl, "Page %d", coloridx);

	GtkWidget *frame = gtk_fixed_new();
	gtk_container_set_border_width(GTK_CONTAINER(frame), 10);
	g_object_set(frame, "user-data", this, NULL);
	gtk_widget_show(frame);

	GdkColor color;
	gdk_color_parse (colornames[coloridx], &color);
	//gtk_widget_modify_bg(frame, GTK_STATE_NORMAL, &color);

	gtk_container_add(GTK_CONTAINER(frame), pane_box->get_widget());
	gtk_widget_modify_bg(pane_box->get_widget(), GTK_STATE_NORMAL, &color);
	
	GtkWidget *label = gtk_label_new(bufferf);
	gtk_container_add(GTK_CONTAINER(pane_box->get_widget()), label);
	gtk_widget_show(label);

	// the tab label
	GtkWidget *tabbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(tabbox);

	GtkWidget *tab_go_button = gtk_button_new_with_label("O");
	gtk_box_pack_start(GTK_BOX(tabbox), tab_go_button, FALSE, FALSE, 0);
	g_signal_connect(GTK_BUTTON(tab_go_button), "clicked", G_CALLBACK(tab_go_cb), this);
	gtk_widget_show(tab_go_button);

	gtk_box_pack_start(GTK_BOX(tabbox), tab_box->get_widget(), FALSE, FALSE, 0);
	gtk_widget_modify_bg(tab_box->get_widget(), GTK_STATE_NORMAL, &color);

	GtkWidget *tab_x_button = gtk_button_new_with_label("X");
	gtk_box_pack_start(GTK_BOX(tabbox), tab_x_button, FALSE, FALSE, 0);
	gtk_widget_show(tab_x_button);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, tabbox);
}

void Page::map_pane()
{
	pane_box->switch_page(true);
}

void Page::unmap_pane()
{
	pane_box->switch_page(false);
}

void Page::tab_transfer_start()
{
	pane_box->transfer(true);
}

void Page::tab_transfer_complete()
{
	pane_box->transfer(false);
}

bool Page::has_tab_key(DeedKey key)
{
	return tab_box->get_deed_key() == key;
}

void Page::refresh_tab_key()
{
	tab_box->refresh_deed_key();
}

void Page::tab_go_cb(GtkWidget *widget, gpointer data)
{
	((Page*) data)->tab_go();
}

void Page::tab_go()
{
	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), find_my_page_num());
}

gint Page::find_my_page_num()
{
	GtkNotebook *nb = GTK_NOTEBOOK(notebook);
	for (gint i = 0; i<gtk_notebook_get_n_pages(nb); i++)
	{
		GtkWidget *page_frame = gtk_notebook_get_nth_page(nb, i);
		Page *page;
		g_object_get(page_frame, "user-data", &page, NULL);
		
		if (page == this)
		{
			return i;
		}
	}
	lite_assert(false);	// page not found!
	return -1;
}
