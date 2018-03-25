/*
 * Copyright (C) 2007 Tilman Sauerbeck (tilman at code-monkey de)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gtk/gtk.h>
#include <poppler.h>

#include <limits.h>
#include <stdlib.h>
#include <stdbool.h>

#include "rakt-window.h"

typedef struct {
	GtkWidget *content_vbox;

	/* Menu & toolbar */
	GtkUIManager *ui_manager;
	GtkAction *action_prev;
	GtkAction *action_next;

	GtkAction *action_zoom_in;
	GtkAction *action_zoom_out;
	GtkAction *action_zoom_100;

	GtkWidget *drawing_area;
	PopplerDocument *document;
	PopplerPage *page;
	gint page_no;
	gdouble scale;
} RaktWindowPriv;

static void window_finalize (GObject *object);

static void on_add_widget (GtkUIManager *merge, GtkWidget *widget, RaktWindow *window);

static void on_action_quit (GtkAction *action, RaktWindow *window);
static void on_action_open (GtkAction *action, RaktWindow *window);
static void on_action_go_next (GtkAction *action, RaktWindow *window);
static void on_action_go_previous (GtkAction *action, RaktWindow *window);
static void on_action_zoom (GtkAction *action, RaktWindow *window);
static void on_action_about (GtkAction *action, RaktWindow *window);

static const GtkActionEntry action_entries[] = {
	{
		"FileMenu", NULL, "_File", NULL, NULL, NULL
	},
	{
		"GoMenu", NULL, "_Go", NULL, NULL, NULL
	},
	{
		"ViewMenu", NULL, "_View", NULL, NULL, NULL
	},
	{
		"HelpMenu", NULL, "_Help", NULL, NULL, NULL
	},
	{
		"Open", GTK_STOCK_OPEN, "_Open",
		"<control>O", "Open a PDF file",
		G_CALLBACK (on_action_open)
	},
	{
		"Next", GTK_STOCK_GO_FORWARD, "_Next",
		"<control>n", "Next",
		G_CALLBACK (on_action_go_next)
	},
	{
		"Previous", GTK_STOCK_GO_BACK, "_Previous",
		"<control>p", "Previous",
		G_CALLBACK (on_action_go_previous)
	},
	{
		"Zoom In", GTK_STOCK_ZOOM_IN, "Zoom _In",
		"<control>plus", "Zoom In",
		G_CALLBACK (on_action_zoom)
	},
	{
		"Zoom Out", GTK_STOCK_ZOOM_OUT, "Zoom _Out",
		"<control>minus", "Zoom Out",
		G_CALLBACK (on_action_zoom)
	},
	{
		"Zoom 100", GTK_STOCK_ZOOM_100, "Zoom 100%",
		NULL, "Zoom 100%",
		G_CALLBACK (on_action_zoom)
	},
	{
		"Quit", GTK_STOCK_QUIT, "_Quit",
		"<control>Q", "Quit the application",
		G_CALLBACK (on_action_quit)
	},
	{
		"About", GTK_STOCK_ABOUT, "_About",
		NULL, "About this application",
		G_CALLBACK (on_action_about)
	}
};

static const gchar *ui_layout =
	"<ui>"
	"	<menubar name='MenuBar'>"
	"		<menu action='FileMenu'>"
	"			<menuitem action='Open'/>"
	"			<separator action='Sep1'/>"
	"			<menuitem action='Quit'/>"
	"		</menu>"
	"		<menu action='GoMenu'>"
	"			<menuitem action='Previous'/>"
	"			<menuitem action='Next'/>"
	"		</menu>"
	"		<menu action='ViewMenu'>"
	"			<menuitem action='Zoom In'/>"
	"			<menuitem action='Zoom Out'/>"
	"			<menuitem action='Zoom 100'/>"
	"		</menu>"
	"		<menu action='HelpMenu'>"
	"			<menuitem action='About'/>"
	"		</menu>"
	"	</menubar>"
	"	<toolbar name='ToolBar'>"
	"		<toolitem action='Open'/>"
	"		<toolitem action='Previous'/>"
	"		<toolitem action='Next'/>"
	"		<toolitem action='Zoom In'/>"
	"		<toolitem action='Zoom Out'/>"
	"		<toolitem action='Zoom 100'/>"
	"	</toolbar>"
	"</ui>";

G_DEFINE_TYPE (RaktWindow, rakt_window, GTK_TYPE_WINDOW)

#define GET_PRIV(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), RAKT_TYPE_WINDOW, RaktWindowPriv))

static void
rakt_window_class_init (RaktWindowClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = window_finalize;

	g_type_class_add_private (object_class, sizeof (RaktWindowPriv));
}

static void
render_page (RaktWindow *window)
{
	RaktWindowPriv *priv;
	double tmpw, tmph;
	int width, height, n_pages;

	priv = GET_PRIV (window);

	n_pages = poppler_document_get_n_pages (priv->document);
	gtk_action_set_sensitive (priv->action_prev, priv->page_no > 0);
	gtk_action_set_sensitive (priv->action_next, priv->page_no < n_pages - 1);

	gtk_action_set_sensitive (priv->action_zoom_in, priv->scale < 3.0);
	gtk_action_set_sensitive (priv->action_zoom_out, priv->scale > 0.4);
	gtk_action_set_sensitive (priv->action_zoom_100, priv->scale != 1.0);

	priv->page = poppler_document_get_page (priv->document, priv->page_no);

	poppler_page_get_size (priv->page, &tmpw, &tmph);

	width = (int) (tmpw * priv->scale + 0.5);
	height = (int) (tmph * priv->scale + 0.5);

	gtk_widget_set_size_request (priv->drawing_area, width, height);
	gtk_widget_queue_draw_area (priv->drawing_area, 0, 0, width, height);
}

void
rakt_window_open_document (RaktWindow *window, const gchar *uri)
{
	RaktWindowPriv *priv;
	GError *error = NULL;

	priv = GET_PRIV (window);

	priv->document = poppler_document_new_from_file (uri, NULL, &error);

	priv->page_no = 0;
	render_page (window);
}

static void
window_create_menu (RaktWindow *window)
{
	RaktWindowPriv *priv;
	GtkActionGroup *action_group;
	GtkAccelGroup *accel_group;
	GError *error = NULL;

	priv = GET_PRIV (window);

	priv->ui_manager = gtk_ui_manager_new ();

	g_signal_connect (priv->ui_manager,
	                  "add_widget",
	                  G_CALLBACK (on_add_widget),
	                  window);

	action_group = gtk_action_group_new ("Actions");
	gtk_action_group_add_actions (action_group, action_entries,
	                              G_N_ELEMENTS (action_entries), window);
	gtk_ui_manager_insert_action_group (priv->ui_manager, action_group, 0);

	accel_group = gtk_ui_manager_get_accel_group (priv->ui_manager);
	gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

	g_object_unref (action_group);

	gtk_ui_manager_add_ui_from_string (priv->ui_manager, ui_layout, -1, &error);

	if (error)
		g_error ("Couldn't create UI: %s\n", error->message);

	gtk_ui_manager_ensure_update (priv->ui_manager);

	priv->action_prev = gtk_ui_manager_get_action (priv->ui_manager,
	                                               "ui/ToolBar/Previous");
	priv->action_next = gtk_ui_manager_get_action (priv->ui_manager,
	                                               "ui/ToolBar/Next");
	priv->action_zoom_in = gtk_ui_manager_get_action (priv->ui_manager,
	                                                  "ui/ToolBar/Zoom In");
	priv->action_zoom_out = gtk_ui_manager_get_action (priv->ui_manager,
	                                                   "ui/ToolBar/Zoom Out");
	priv->action_zoom_100 = gtk_ui_manager_get_action (priv->ui_manager,
	                                                   "ui/ToolBar/Zoom 100");

	gtk_action_set_sensitive (priv->action_prev, false);
	gtk_action_set_sensitive (priv->action_next, false);

	gtk_action_set_sensitive (priv->action_zoom_in, false);
	gtk_action_set_sensitive (priv->action_zoom_out, false);
	gtk_action_set_sensitive (priv->action_zoom_100, false);
}

static gboolean
on_expose_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	RaktWindowPriv *priv;
	cairo_t *cr;

	priv = GET_PRIV (user_data);

	cr = gdk_cairo_create (priv->drawing_area->window);

	cairo_set_source_rgb (cr, 255, 255, 255);
	cairo_paint (cr);

	cairo_scale (cr, priv->scale, priv->scale);

	if (priv->page)
		poppler_page_render (priv->page, cr);

	cairo_destroy (cr);

	return FALSE;
}

static gboolean
on_delete_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	gtk_main_quit ();

	return TRUE;
}

static void
rakt_window_init (RaktWindow *window)
{
	RaktWindowPriv *priv;
	GtkWidget *scrolled;

	priv = GET_PRIV (window);

	priv->scale = 1.0;

	gtk_window_set_default_size (GTK_WINDOW (window), 640, 480);
	gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);

	priv->content_vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (priv->content_vbox);
	gtk_container_add (GTK_CONTAINER (window), priv->content_vbox);

	window_create_menu (window);

	priv->drawing_area = gtk_drawing_area_new ();
	gtk_widget_show (priv->drawing_area);

	g_signal_connect (priv->drawing_area, "expose-event",
	                  G_CALLBACK (on_expose_event), window);

	scrolled = gtk_scrolled_window_new (NULL, NULL);

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
	                                GTK_POLICY_AUTOMATIC,
	                                GTK_POLICY_AUTOMATIC);

	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled),
	                                       priv->drawing_area);
	gtk_widget_show (scrolled);

	gtk_box_pack_start (GTK_BOX (priv->content_vbox), scrolled,
	                    TRUE, TRUE, 0);

	g_signal_connect (GTK_WINDOW (window), "delete-event",
	                  G_CALLBACK (on_delete_event), NULL);
}

static void
window_finalize (GObject *object)
{
	RaktWindowPriv *priv;

	priv = GET_PRIV (object);

	g_object_unref (priv->ui_manager);

	G_OBJECT_CLASS (rakt_window_parent_class)->finalize (object);
}

static void
on_add_widget (GtkUIManager *merge, GtkWidget *widget, RaktWindow *window)
{
	RaktWindowPriv *priv;

	priv = GET_PRIV (window);

	gtk_box_pack_start (GTK_BOX (priv->content_vbox), widget, FALSE, FALSE, 0);
}

static void
on_action_quit (GtkAction *action, RaktWindow *window)
{
	gtk_main_quit ();
}

static void
on_action_open (GtkAction *action, RaktWindow *window)
{
	GtkWidget *dialog;
	GtkFileFilter *filter;
	gint n;

	dialog = gtk_file_chooser_dialog_new ("Open PDF",
	                                      GTK_WINDOW (window),
	                                      GTK_FILE_CHOOSER_ACTION_OPEN,
	                                      GTK_STOCK_OPEN,
	                                      GTK_RESPONSE_ACCEPT,
	                                      GTK_STOCK_CANCEL,
	                                      GTK_RESPONSE_CANCEL, NULL);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, "PDF files");
	gtk_file_filter_add_pattern (filter, "*.pdf");

	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

	/* now run the dialog */
	n = gtk_dialog_run (GTK_DIALOG (dialog));

	if (n == GTK_RESPONSE_ACCEPT) {
		gchar *uri;

		uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));
		rakt_window_open_document (window, uri);
		g_free (uri);
	}

	gtk_widget_destroy (dialog);
}

static void
on_action_go_next (GtkAction *action, RaktWindow *window)
{
	RaktWindowPriv *priv;

	priv = GET_PRIV (window);

	priv->page_no++;
	render_page (window);
}

static void
on_action_go_previous (GtkAction *action, RaktWindow *window)
{
	RaktWindowPriv *priv;

	priv = GET_PRIV (window);

	priv->page_no--;
	render_page (window);
}

static void
on_action_zoom (GtkAction *action, RaktWindow *window)
{
	RaktWindowPriv *priv;

	priv = GET_PRIV (window);

	if (action == priv->action_zoom_in)
		priv->scale += 0.2;
	else if (action == priv->action_zoom_out)
		priv->scale -= 0.2;
	else
		priv->scale = 1.0;

	render_page (window);
}

static void
on_action_about (GtkAction *action, RaktWindow *window)
{
	const gchar *authors[] = {
		"Tilman Sauerbeck",
		NULL
	};

	gtk_show_about_dialog (GTK_WINDOW (window),
	                       "name", PACKAGE_NAME,
	                       "copyright", "Copyright (c) 2007 Tilman Sauerbeck",
	                       "version", VERSION,
	                       "authors", authors,
	                       NULL);
}

GtkWidget *
rakt_window_new (void)
{
	return g_object_new (RAKT_TYPE_WINDOW, NULL);
}
