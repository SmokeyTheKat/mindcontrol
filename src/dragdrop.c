#include <mindcontrol/dragdrop.h>

#include <gtk/gtk.h>
#include <glib.h>

#include <mindcontrol/device_control.h>

#define TYPE "text/uri-list"

static GtkWidget* window;

static void drag_get_data(GtkWidget* widget, GdkDragContext* context,
								 GtkSelectionData* data, guint info, guint time, char* filename)
{
	char* uris[] = {
		g_filename_to_uri(filename, 0, 0),
		0,
	};
	gtk_selection_data_set_uris(
		data,
		uris
	);
}

static void drag_end(GtkWidget* widget)
{
	gtk_window_close(GTK_WINDOW(window));
}

static void drag_start(GtkWidget* widget)
{
	GtkTargetList* target_list = gtk_target_list_new(0, 0);
	gtk_target_list_add(target_list, gdk_atom_intern(TYPE, true), 0, 5);

	gtk_drag_begin_with_coordinates(
		widget,
		target_list,
		GDK_ACTION_COPY,
		1,
		0,
		0, 0
	);
	gtk_window_move(GTK_WINDOW(window), 0, 0);
}

static bool activate(char* filename)
{
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window), 10, 10);
	gtk_window_set_resizable(GTK_WINDOW(window), false);
	gtk_widget_set_hexpand(window, false);

	struct vec pos = device_control_cursor_get();
	gtk_window_move(GTK_WINDOW(window), pos.x, pos.y);

//    g_signal_connect(window, "button-press-event", G_CALLBACK(drag_start), 0);
	g_signal_connect(window, "enter-notify-event", G_CALLBACK(drag_start), 0);
	g_signal_connect(window, "drag-end", G_CALLBACK(drag_end), 0);
	g_signal_connect(window, "drag-data-get", G_CALLBACK(drag_get_data), filename);

	gtk_widget_show_all(window);
	return 0;
}

void set_dragdrop_files(char* filename)
{
	g_idle_add(activate, filename);
//    GtkApplication* app = gtk_application_new("com.test.yo", G_APPLICATION_FLAGS_NONE);
//    activate(app, filename);
//    g_signal_connect(app, "activate", G_CALLBACK(activate), filename);
//    g_application_run(G_APPLICATION(app), 0, 0);
}