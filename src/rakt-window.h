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

#ifndef __RAKT_WINDOW_H
#define __RAKT_WINDOW_H

#include <gtk/gtkwindow.h>

G_BEGIN_DECLS

#define RAKT_TYPE_WINDOW \
	(rakt_window_get_type ())
#define RAKT_WINDOW(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), RAKT_TYPE_WINDOW, RaktWindow))
#define RAKT_WINDOW_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), RAKT_TYPE_WINDOW, RaktWindowClass))
#define RAKT_IS_WINDOW(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), RAKT_TYPE_WINDOW))
#define RAKT_IS_WINDOW_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), RAKT_TYPE_WINDOW))
#define RAKT_WINDOW_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), RAKT_TYPE_WINDOW, RaktWindowClass))

typedef struct {
	GtkWindow parent_instance;
} RaktWindow;

typedef struct {
	GtkWindowClass parent_class;
	void (*quitting) (RaktWindow *window);
} RaktWindowClass;

GType rakt_window_get_type (void);
GtkWidget *rakt_window_new (void);

void rakt_window_open_document (RaktWindow *window, const gchar *uri);

G_END_DECLS

#endif
