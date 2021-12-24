#include <gtk/gtk.h>
#include <stdbool.h>

#include "client.h"
#include "controler.h"
#include "device_control.h"
#include "vec.h"
#include "list.h"
#include "gui.h"
#include "utils.h"

static void create_screen(GtkWidget* widget, struct vec* pos);

struct gclient 
{
	struct client client;
	GtkWidget* button;
};

struct gclient root = {
	{
		.active=true,
		.sck=-1,
		.ip={0},
		.left=0,
		.right=0,
		.up=0,
		.down=0,
	},
	0,
};
struct client* server_client = &root;

GtkWidget* window;
GtkWidget* client_grid;

static GtkButton* gtk_button_new_with_image_from_file(char* file_path, int width, int height)
{
	GtkWidget* button = gtk_button_new();

	GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file_at_scale(file_path, width, height, false, 0);

	GtkWidget* image = gtk_image_new_from_pixbuf(pixbuf);

	gtk_button_set_image(GTK_BUTTON(button), GTK_IMAGE(image));

	return button;
}

bool screen_add_exists(struct gclient* orig_screen, struct gclient* screen, int x, int y)
{
	struct list seen = make_list(100, struct gclient*);
	struct list queue = make_list(100, struct gclient*);
	list_push_back(&queue, screen, struct gclient*);
	list_push_back(&seen, screen, struct gclient*);
	while (queue.length != 0)
	{
		struct gclient* current = list_first(&queue, struct gclient*);
		list_remove(&queue, 0, struct gclient*);

		if (
				(
					(current->client.pos.x + 1 == x && current->client.pos.y     == y) ||
					(current->client.pos.x - 1 == x && current->client.pos.y     == y) ||
					(current->client.pos.x     == x && current->client.pos.y + 1 == y) ||
					(current->client.pos.x     == x && current->client.pos.y - 1 == y)
				) &&
				current != orig_screen
			)
		{
			return true;
		}

		for (int j = 0; j < 4; j++)
		{
			if (current->client.directions[j] &&
				list_index_of(&seen, current->client.directions[j], struct gclient*) == -1)
			{
				list_push_back(&queue, current->client.directions[j], struct gclient*);
				list_push_back(&seen, current->client.directions[j], struct gclient*);
			}
		}
	}
	return false;
}

static void grid_add_new_screen_button(int x, int y)
{
	GtkWidget* button = gtk_button_new_with_label("+");
	struct vec* pos = malloc(sizeof(struct vec));
	pos->x = x;
	pos->y = y;
	g_signal_connect(button, "clicked", G_CALLBACK(create_screen), pos);
	gtk_grid_attach(GTK_GRID(client_grid), button, pos->x, pos->y, 1, 1);
}

static void screen_add_new_screen_buttons(struct gclient* screen)
{
	if (!screen->client.left && !screen_add_exists(screen, &root, screen->client.pos.x - 1, screen->client.pos.y))
	{
		grid_add_new_screen_button(screen->client.pos.x - 1, screen->client.pos.y);
	}

	if (!screen->client.right && !screen_add_exists(screen, &root, screen->client.pos.x + 1, screen->client.pos.y))
	{
		grid_add_new_screen_button(screen->client.pos.x + 1, screen->client.pos.y);
	}

	if (!screen->client.down && !screen_add_exists(screen, &root, screen->client.pos.x, screen->client.pos.y + 1))
	{
		grid_add_new_screen_button(screen->client.pos.x, screen->client.pos.y + 1);
	}

	if (!screen->client.up && !screen_add_exists(screen, &root, screen->client.pos.x, screen->client.pos.y - 1))
	{
		grid_add_new_screen_button(screen->client.pos.x, screen->client.pos.y - 1);
	}

	gtk_widget_show_all(window);
}

static void screen_assign_neighbors(struct gclient* screen)
{
	struct gclient* try_left = client_find_by_pos(&root, screen->client.pos.x - 1, screen->client.pos.y);
	struct gclient* try_right = client_find_by_pos(&root, screen->client.pos.x + 1, screen->client.pos.y);
	struct gclient* try_up = client_find_by_pos(&root, screen->client.pos.x, screen->client.pos.y - 1);
	struct gclient* try_down = client_find_by_pos(&root, screen->client.pos.x, screen->client.pos.y + 1);

	if (try_left)
	{
		try_left->client.right = screen;
		screen->client.left = try_left;
	}
	if (try_right)
	{
		try_right->client.left = screen;
		screen->client.right = try_right;
	}
	if (try_up)
	{
		try_up->client.down = screen;
		screen->client.up = try_up;
	}
	if (try_down)
	{
		try_down->client.up = screen;
		screen->client.down = try_down;
	}
}

struct set_client_ip
{
	struct gclient* screen;
	GtkWidget* textbox;
};

static void edit_client_set_screen_data(GtkWidget* widget, gint response, struct set_client_ip* data)
{
	GtkEntry* entry = data->textbox;
	const char* textbox_data = gtk_entry_get_text(entry);
	strcpy(data->screen->client.ip, textbox_data);
	gtk_widget_destroy(widget);
	free(data);
}

static void edit_client(GtkWidget* widget, struct gclient* screen)
{
	static GtkWidget* textbox;

	GtkWidget* dialog = gtk_dialog_new_with_buttons("Get Text",
										  GTK_WINDOW(window),
										  GTK_DIALOG_MODAL,
										  GTK_STOCK_OK,
										  GTK_RESPONSE_OK,
										  NULL);

	GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	GtkWidget* grid = gtk_grid_new();
	gtk_container_add(GTK_CONTAINER(content_area), grid);


	GtkWidget* label = gtk_label_new("ip: ");
	gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

	textbox = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(textbox), screen->client.ip);
	gtk_grid_attach(GTK_GRID(grid), textbox, 1, 0, 1, 1);


	GtkWidget* button = gtk_button_new_with_label("remove");
	gtk_grid_attach(GTK_GRID(grid), button, 0, 1, 2, 1);


	gtk_widget_show_all(dialog);

	struct set_client_ip* sci = malloc(sizeof(struct set_client_ip));
	sci->screen = screen;
	sci->textbox = textbox;

	g_signal_connect(GTK_DIALOG(dialog), "response", G_CALLBACK(edit_client_set_screen_data), sci);
}

static void create_screen(GtkWidget* widget, struct vec* pos)
{
	struct gclient* new_screen = calloc(sizeof(struct gclient), 1);
	new_screen->client.pos = *pos;
	new_screen->button = gtk_button_new_with_image_from_file("./monitor.png", 80, 80);

	g_signal_connect(new_screen->button, "clicked", G_CALLBACK(edit_client), new_screen);
	gtk_grid_attach(GTK_GRID(client_grid), new_screen->button, new_screen->client.pos.x, new_screen->client.pos.y, 1, 1);
	gtk_container_remove(GTK_CONTAINER(client_grid), widget);

	screen_assign_neighbors(new_screen);

	screen_add_new_screen_buttons(new_screen);

	gtk_widget_show_all(window);
}

static GtkWidget* generate_client_grid(void)
{
	GtkWidget* grid = gtk_grid_new();

	gtk_grid_set_row_homogeneous(GTK_GRID(grid), true);
	gtk_grid_set_column_homogeneous(GTK_GRID(grid), true);

	gtk_grid_attach(GTK_GRID(grid), root.button, root.client.pos.x, root.client.pos.y, 1, 1);

	return grid;
}

CREATE_THREAD(run_controller, int, port, {
	controler_init(port);
})

THREAD instance_thread;
bool instance_running = false;

struct menu_options
{
	GtkButton* button_start;
	GtkEntry* entry_port;
};

static void toggle_controller(GtkWidget* widget, struct menu_options* menu_options)
{
	static int* port = 0;
	if (port == 0) port = malloc(sizeof(int));

	instance_running = !instance_running;
	if (instance_running)
	{
		gtk_button_set_label(menu_options->button_start, "stop");

		instance_thread = MAKE_THREAD();
		*port = atoi(gtk_entry_get_text(menu_options->entry_port));
		THREAD_CALL(&instance_thread, run_controller, port);
	}
	else
	{
		gtk_button_set_label(menu_options->button_start, "start");

		control_state.state = CONTROL_STATE_QUIT;
		for (int i = 0; i < 100; i++)
			SLEEP(REST_TIME);
		THREAD_KILL(&instance_thread);
	}
}

static GtkWidget* generate_menu_controls(void)
{
	GtkWidget* menu = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	GtkWidget* label_ip = gtk_label_new_with_mnemonic(device_control_get_ip());
	GtkWidget* label_hostname = gtk_label_new_with_mnemonic(device_control_get_hostname());

	GtkWidget* entry_port = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry_port), "1234");

	GtkWidget* button_start = gtk_button_new_with_label("start");

	static struct menu_options menu_options;
	menu_options.button_start = button_start;
	menu_options.entry_port = entry_port;

	g_signal_connect(button_start, "clicked", G_CALLBACK(toggle_controller), &menu_options);

	gtk_box_pack_start(GTK_BOX(menu), label_ip, true, false, 0);
	gtk_box_pack_start(GTK_BOX(menu), label_hostname, true, false, 0);
	gtk_box_pack_start(GTK_BOX(menu), entry_port, true, false, 0);
	gtk_box_pack_start(GTK_BOX(menu), button_start, true, false, 0);

	return menu;
}

static GtkWidget* generate_server_page(void)
{
	client_grid = generate_client_grid();
	screen_add_new_screen_buttons(&root);

	GtkWidget* menu = generate_menu_controls();

	GtkWidget* paned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
	gtk_paned_pack1(GTK_PANED(paned), client_grid, false, false);
	gtk_paned_pack2(GTK_PANED(paned), menu, false, false);

	return paned;
}

static GtkWidget* generate_client_page(void)
{
	GtkWidget* paned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
	gtk_paned_pack1(GTK_PANED(paned), gtk_button_new_with_label("aaaaaaaaaa"), false, false);
	gtk_paned_pack2(GTK_PANED(paned), gtk_button_new_with_label("bbbbbbbbbb"), false, false);
	return paned;
}

static void activate(GtkApplication *app, gpointer user_data)
{
	window = gtk_application_window_new(app);
	gtk_window_set_keep_above(GTK_WINDOW(window), true);
	gtk_window_set_title(GTK_WINDOW(window), "Window");
	gtk_window_set_default_size(GTK_WINDOW(window), 0, 0);
	gtk_window_set_resizable(GTK_WINDOW(window), false);
	gtk_widget_set_hexpand(window, false);
	gtk_container_set_border_width(GTK_CONTAINER(window), 10);

	root.button = gtk_button_new_with_image_from_file("./controller.png", 80, 80);

	GtkWidget* notebook_tabs = gtk_notebook_new();

	GtkWidget* server_page = generate_server_page();
	GtkWidget* client_page = generate_client_page();

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook_tabs), server_page, gtk_label_new_with_mnemonic("server"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook_tabs), client_page, gtk_label_new_with_mnemonic("client"));

	gtk_container_add(GTK_CONTAINER(window), notebook_tabs);

	gtk_widget_show_all(window);
}


int gui_main(void)
{
	root.client.pos.x = 100;
	root.client.pos.y = 100;

	GtkApplication* app = gtk_application_new("org.gtk.example", G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app, "activate", G_CALLBACK (activate), NULL);
	char* argv[] = {"mindcontrol"};
	int status = g_application_run(G_APPLICATION (app), 1, argv);
	g_object_unref (app);
	return status;
}