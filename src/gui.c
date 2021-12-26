#include <gtk/gtk.h>
#include <glib.h>

#include "client.h"
#include "controler.h"
#include "device_control.h"
#include "vec.h"
#include "list.h"
#include "gui.h"
#include "utils.h"
#include "ddcSocket.h"

#define LAST_WAS_CLIENT 1
#define LAST_WAS_SERVER 2

static void create_screen(GtkWidget* widget, struct vec* pos);

struct gclient 
{
	struct client client;
	GtkWidget* button;
};

struct client_info
{
	char ip[16];
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
struct list clients = {0};

static GtkWidget* window;
static GtkWidget* client_grid;
static GtkWidget* edit_client_textbox;
static GtkWidget* entry_server_ip;
static GtkWidget* client_scan_dialog;
static GtkWidget* server_scan_dialog;

static GtkPaned* gtk_labeled_new_with_widget(char* label, GtkWidget* widget)
{
	GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(box), gtk_label_new_with_mnemonic(label), false, false, 0);
	gtk_box_pack_start(GTK_BOX(box), widget, true, true, 0);

	return box;
}

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
			free_list(&seen);
			free_list(&queue);
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
	free_list(&seen);
	free_list(&queue);
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
	const char* textbox_data = gtk_combo_box_text_get_active_text(entry);
	strcpy(data->screen->client.ip, textbox_data);
	gtk_widget_destroy(widget);
	free(data);
}

static void edit_client(GtkWidget* widget, struct gclient* screen)
{
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

	edit_client_textbox = gtk_combo_box_text_new_with_entry();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(edit_client_textbox), screen->client.ip);
	gtk_combo_box_set_active(GTK_COMBO_BOX_TEXT(edit_client_textbox), 0);

	for (list_iterate(&clients, i, struct client_info))
	{
		printf("client: %s\n", i->ip);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(edit_client_textbox), i->ip);
	}
	
	gtk_grid_attach(GTK_GRID(grid), edit_client_textbox, 1, 0, 1, 1);


	GtkWidget* button = gtk_button_new_with_label("remove");
	gtk_grid_attach(GTK_GRID(grid), button, 0, 1, 2, 1);


	gtk_widget_show_all(dialog);

	struct set_client_ip* sci = malloc(sizeof(struct set_client_ip));
	sci->screen = screen;
	sci->textbox = edit_client_textbox;

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

THREAD instance_thread;
bool instance_running = false;

struct menu_options
{
	int last;
	GtkButton* button_start;
	GtkEntry* entry_port;
};

struct client_menu_options
{
	int last;
	GtkButton* button_start;
	GtkEntry* entry_port;
	GtkEntry* entry_ip;
};

struct client_options
{
	char ip[16];
	int port;
};

CREATE_THREAD(run_controller, int, port, {
	controler_init(port);
})

CREATE_THREAD(run_client, struct client_options, options, {
	receiver_init(options.ip, options.port);
})

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

static void toggle_client(GtkWidget* widget, struct client_menu_options* menu_options)
{
	static struct client_options* client_options = 0;
	if (client_options == 0) client_options = malloc(sizeof(struct client_options));
	strcpy(client_options->ip, gtk_entry_get_text(menu_options->entry_ip));
	client_options->port = atoi(gtk_entry_get_text(menu_options->entry_port));

	instance_running = !instance_running;
	if (instance_running)
	{
		gtk_button_set_label(menu_options->button_start, "stop");

		instance_thread = MAKE_THREAD();
		THREAD_CALL(&instance_thread, run_client, client_options);
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

bool server_scan_end(void* _)
{
	gtk_widget_destroy(server_scan_dialog);
	return false;
}

bool server_scan(void* _)
{
	(void)_;
	char ip[16];
	strcpy(ip, device_control_get_ip());
	for (int i = 0; i < 255; i++)
	{
		int j;
		int dc;
		for (j = 0, dc = 0; j < 16 && dc < 3; j++)
			if (ip[j] == '.') dc++;
		ip[j] = 0;

		char numstr[6];
		sprintf(numstr, "%d", i);
		strcat(ip, numstr);

		printf("trying %s:%d\n", ip, 6969);

		struct dsocket_tcp_client cli = make_dsocket_tcp_client(ip, 6969);
		if (dsocket_tcp_client_connect(&cli) != 0)
		{
			close(cli.dscr);
			continue;
		}
		if (clients.data == 0)
		{
			clients = make_list(4, struct client_info);
		}
		char* ip = inet_ntoa(cli.server.sin_addr);
		struct client_info client_info;
		strcpy(client_info.ip, ip);
		list_push_back(&clients, client_info, struct client_info);
		printf("found %s!\n", ip);
		close(cli.dscr);
	}
	g_idle_add(server_scan_end, 0);
}

static void display_server_scan(GtkWidget* widget, struct gclient* _)
{
	(void)_;
	printf("scanning for clients...\n");
	server_scan_dialog = gtk_dialog_new_with_buttons("Get Text",
										  GTK_WINDOW(window),
										  GTK_DIALOG_MODAL,
										  GTK_STOCK_OK,
										  GTK_RESPONSE_OK,
										  NULL);

	GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(server_scan_dialog));

	GtkWidget* spinner = gtk_spinner_new();
	gtk_spinner_start(GTK_SPINNER(spinner));

	GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(box), gtk_label_new_with_mnemonic("searching for controller..."), false, false, 0);
	gtk_box_pack_start(GTK_BOX(box), spinner, true, true, 0);

	gtk_container_add(GTK_CONTAINER(content_area), box);

	gtk_widget_show_all(server_scan_dialog);

	g_thread_new("thread", server_scan, 0);
}

static void save_config(GtkWidget* widget, struct menu_options* menu_options)
{
	const struct vec master_pos = {100, 100};
	FILE* fp = fopen("./mindcontrol.conf", "w");
	printf("omgh i\n");
	int port = atoi(gtk_entry_get_text(menu_options->entry_port));
	fprintf(fp, "port %d\n", port);
	fprintf(fp, "ip %s\n", gtk_entry_get_text(entry_server_ip));
	fprintf(fp, "last %d\n", menu_options->last);
	ITERATE_OVER_CLIENTS({
		if (!vec_compare(current->pos, master_pos))
			fprintf(fp, "display %d %d %s\n", current->pos.x, current->pos.y, current->ip);
	});
	fclose(fp);
}

static void load_config(void)
{
	FILE* fp = fopen("./mindcontrol.conf", "r");
	char buffer[4096];
	
	int port;
	char controller_ip[16];
	int last;
	while (fgets(buffer, sizeof(buffer), fp))
	{
		printf("[%s]\n", buffer);
		int x, y;
		char ip[16];
		if (!strncmp(buffer, "port", 4))
			sscanf(buffer, "port %d", &port);
		else if (!strncmp(buffer, "ip", 2))
			sscanf(buffer, "ip %s", controller_ip);
		else if (!strncmp(buffer, "last", 4))
			sscanf(buffer, "last %d", &last);
		else if (!strncmp(buffer, "display", 7))
		{
			sscanf(buffer, "display %d %d %s", &x, &y, ip);
			printf("%d %d %s\n", x, y, ip);
			struct vec pos = {x, y};
			create_screen(gtk_grid_get_child_at(GTK_GRID(client_grid), x, y), &pos);
			struct client* client = client_find_by_pos(server_client, x, y);
			strcpy(client->ip, ip);
		}
	}
	gtk_entry_buffer_set_text(gtk_entry_get_buffer(entry_server_ip), controller_ip, strlen(controller_ip));
	printf("port: %d\n", port);
}

static GtkWidget* generate_server_menu_controls(void)
{
	GtkWidget* menu = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_set_homogeneous(GTK_BOX(menu), false);

	GtkWidget* label_my_ip = gtk_label_new_with_mnemonic(device_control_get_ip());
	GtkWidget* labeled_label_my_ip = gtk_labeled_new_with_widget("Computer IP: ", label_my_ip);

	GtkWidget* label_my_hostname = gtk_label_new_with_mnemonic(device_control_get_hostname());
	GtkWidget* labeled_label_my_hostname = gtk_labeled_new_with_widget("Computer name: ", label_my_hostname);

	GtkWidget* entry_server_port = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry_server_port), "1234");
	GtkWidget* labeled_entry_server_port = gtk_labeled_new_with_widget("Controller port:", entry_server_port);

	GtkWidget* scan_start = gtk_button_new_with_label("pair");
	g_signal_connect(scan_start, "clicked", G_CALLBACK(display_server_scan), 0);

	GtkWidget* button_start = gtk_button_new_with_label("start");
	static struct menu_options menu_options;
	menu_options.last = LAST_WAS_SERVER;
	menu_options.button_start = button_start;
	menu_options.entry_port = entry_server_port;
	g_signal_connect(button_start, "clicked", G_CALLBACK(toggle_controller), &menu_options);

	GtkWidget* button_save_config = gtk_button_new_with_label("save config");
	g_signal_connect(button_save_config, "clicked", G_CALLBACK(save_config), &menu_options);

	gtk_box_pack_start(GTK_BOX(menu), labeled_label_my_ip, false, false, 0);
	gtk_box_pack_start(GTK_BOX(menu), labeled_label_my_hostname, false, false, 0);
	gtk_box_pack_start(GTK_BOX(menu), labeled_entry_server_port, false, false, 0);
	gtk_box_pack_start(GTK_BOX(menu), scan_start, false, false, 0);
	gtk_box_pack_start(GTK_BOX(menu), button_save_config, false, false, 0);
	gtk_box_pack_start(GTK_BOX(menu), button_start, false, false, 0);

	return menu;
}

static GtkWidget* generate_server_page(void)
{
	client_grid = generate_client_grid();
	screen_add_new_screen_buttons(&root);

	GtkWidget* menu = generate_server_menu_controls();

	GtkWidget* paned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
	gtk_paned_pack1(GTK_PANED(paned), client_grid, false, false);
	gtk_paned_pack2(GTK_PANED(paned), menu, false, false);

	return paned;
}

bool client_update_controller_ip(char* ip)
{
	gtk_entry_buffer_set_text(gtk_entry_get_buffer(entry_server_ip), ip, strlen(ip));
	gtk_widget_destroy(client_scan_dialog);

	return false;
}


bool client_scan(void* _)
{
	(void)_;
	struct dsocket_tcp_server srv = make_dsocket_tcp_server(6969);
	dsocket_tcp_server_bind(&srv);
	dsocket_tcp_server_start_listen(&srv);
	while (1)
	{
		dsocket_tcp_server_listen(&srv);
		char* ip = inet_ntoa(srv.server.sin_addr);
		g_idle_add(client_update_controller_ip, ip);
	}
}

static void display_client_scan(GtkWidget* widget, struct gclient* _)
{
	(void)_;
	printf("scanning for controller...\n");
	client_scan_dialog = gtk_dialog_new_with_buttons("Get Text",
										  GTK_WINDOW(window),
										  GTK_DIALOG_MODAL,
										  GTK_STOCK_OK,
										  GTK_RESPONSE_OK,
										  NULL);

	GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(client_scan_dialog));

	GtkWidget* spinner = gtk_spinner_new();
	gtk_spinner_start(GTK_SPINNER(spinner));

	GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(box), gtk_label_new_with_mnemonic("searching for controller..."), false, false, 0);
	gtk_box_pack_start(GTK_BOX(box), spinner, true, true, 0);

	gtk_container_add(GTK_CONTAINER(content_area), box);

	gtk_widget_show_all(client_scan_dialog);

	g_thread_new("thread", client_scan, 0);
}

static GtkWidget* generate_client_menu_controls(void)
{
	GtkWidget* menu = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_set_homogeneous(GTK_BOX(menu), false);

	GtkWidget* label_my_ip = gtk_label_new_with_mnemonic(device_control_get_ip());
	GtkWidget* labeled_label_my_ip = gtk_labeled_new_with_widget("Computer IP:", label_my_ip);

	GtkWidget* label_my_hostname = gtk_label_new_with_mnemonic(device_control_get_hostname());
	GtkWidget* labeled_label_my_hostname = gtk_labeled_new_with_widget("Computer name:", label_my_hostname);

	entry_server_ip = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry_server_ip), "0.0.0.0");
	GtkWidget* labeled_entry_server_ip = gtk_labeled_new_with_widget("Controller IP:", entry_server_ip);

	GtkWidget* entry_server_port = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry_server_port), "1234");
	GtkWidget* labeled_entry_server_port = gtk_labeled_new_with_widget("Controller port:", entry_server_port);

	GtkWidget* scan_start = gtk_button_new_with_label("pair");
	g_signal_connect(scan_start, "clicked", G_CALLBACK(display_client_scan), 0);

	GtkWidget* button_start = gtk_button_new_with_label("start");
	static struct client_menu_options menu_options;
	menu_options.last = LAST_WAS_CLIENT;
	menu_options.button_start = button_start;
	menu_options.entry_ip = entry_server_ip;
	menu_options.entry_port = entry_server_port;
	g_signal_connect(button_start, "clicked", G_CALLBACK(toggle_client), &menu_options);

	GtkWidget* button_save_config = gtk_button_new_with_label("save config");
	g_signal_connect(button_save_config, "clicked", G_CALLBACK(save_config), &menu_options);

	gtk_box_pack_start(GTK_BOX(menu), labeled_label_my_ip, false, false, 0);
	gtk_box_pack_start(GTK_BOX(menu), labeled_label_my_hostname, false, false, 0);
	gtk_box_pack_start(GTK_BOX(menu), labeled_entry_server_ip, false, false, 0);
	gtk_box_pack_start(GTK_BOX(menu), labeled_entry_server_port, false, false, 0);
	gtk_box_pack_start(GTK_BOX(menu), scan_start, false, false, 0);
	gtk_box_pack_start(GTK_BOX(menu), button_save_config, false, false, 0);
	gtk_box_pack_start(GTK_BOX(menu), button_start, false, false, 0);

	return menu;
}

static GtkWidget* generate_client_page(void)
{
	return generate_client_menu_controls();
}

static void activate(GtkApplication *app, gpointer user_data)
{
	window = gtk_application_window_new(app);
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

	load_config();

	g_thread_new("thread", client_scan, 0);
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