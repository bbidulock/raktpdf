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

#include <limits.h>
#include <stdlib.h>
#include <gtk/gtkmain.h>

#include "rakt-window.h"

int
main (int argc, char **argv)
{
	GtkWidget *window;
	gchar rp[PATH_MAX], filename[PATH_MAX];

	gtk_init (&argc, &argv);

	g_set_application_name (PACKAGE_NAME);

	window = rakt_window_new ();
	gtk_widget_show (window);

	if (argc > 1) {
		g_snprintf (filename, sizeof (filename),
		            "file://%s", realpath (argv[1], rp));

		rakt_window_open_document (RAKT_WINDOW (window), filename);
	}

	gtk_main ();

	gtk_widget_destroy (window);

	return 0;
}
