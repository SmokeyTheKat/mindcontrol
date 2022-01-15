#include "controller.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "device_control.h"
#include "commands.h"
#include "ddcSocket.h"
#include "config.h"
#include "screen.h"
#include "client.h"
#include "state.h"
#include "gui.h"
#include "utils.h"
#include "dragdrop.h"

#define MAX_VEL ((struct vec){30, 30})

#define send_command(command, format, ...) _send_command("[" command format "]",  __VA_ARGS__)
static void _send_command(char* fmt, ...);
static void switch_state_to_main(void);
static void switch_to_client_on_edge(int edge_hit);
static void forward_input(void);

static struct dsocket_tcp_server server;

static struct client* active_client;

static unsigned long ping_start = 0;

static data_state(int x; int y; int edge_from) control_state;

char* transfer_client_to_server(struct client* client, char* filepath, long file_length)
{
	char* filename = strrstr(filepath, "/\\") + 1;

	char* tmp_filepath = malloc(2048);
	tmp_filepath[0] = 0;

	strcat(tmp_filepath, "/home/william/.tmp/");
	strcat(tmp_filepath, filename);

	FILE* fp = fopen(tmp_filepath, "wb");
	if (fp == 0)
	{
		printf("BIIIIIIIIIIIGGGGGG ERRRRROR\n");
		return 0;
	}

	char buffer[4096];

	int byte_count;
	int bytes_read = 0;
	while (bytes_read < file_length && (byte_count = dsocket_tcp_server_receive(server, client->sck, buffer, sizeof(buffer))) > 0)
	{
		fwrite(buffer, 1, MIN(byte_count, file_length - bytes_read), fp);
		bytes_read += byte_count;
	}

	fclose(fp);

	return tmp_filepath;
}

struct client* get_client(void)
{
	int sck = dsocket_tcp_server_listen(&server);
	char* ip = inet_ntoa(server.server.sin_addr);
	printf("%s\n", ip);
	
	struct client* client = client_find_by_ip(server_client, ip);
	client->active = true;
	client->sck = sck;
	return client;
}

static void _send_command(char* fmt, ...)
{
	char buffer[1024];

	va_list args;
	va_start(args, fmt);

	vsprintf(buffer, fmt, args);

	va_end(args);

	if (active_client->sck != -1) dsocket_tcp_server_send(server, active_client->sck, buffer, strlen(buffer));
}

void controller_set_state(state_t state)
{
	control_state.state = state;
}

struct client* client_get_client_in_direction(struct client* client, int edge)
{
	if (edge & EDGE_RIGHT)
		if (client->right) return client->right;

	if (edge & EDGE_LEFT)
		if (client->left) return client->left;

	if (edge & EDGE_BOTTOM)
		if (client->down) return client->down;

	if (edge & EDGE_TOP)
		if (client->up) return client->up;

	return 0;
}

CREATE_THREAD(controller_receive, void*, _, {
	(void)_;
	while (1)
	{
		if (active_client->sck == -1 || active_client->active == false)
		{
			SLEEP(REST_TIME);
			continue;
		}

		char buffer[4096] = {0};

		int bytes_read = dsocket_tcp_server_receive(server, active_client->sck, buffer, sizeof(buffer));
		if (bytes_read == 0)
		{
			active_client->sck = -1;
			active_client->active = false;
			control_state.state = CONTROL_STATE_SWITCH_TO_MAIN;
			active_client = server_client;
			continue;
		}
		else if (bytes_read < 0) continue;

		char* data = extract_command(buffer);
		do
		{
			if (IS_COMMAND(COMMAND_TRANSFER_FILE, data))
			{
				char filename[8192];
				data_get_string(&data, filename);
				data -= 3;
				printf("%s\n", data);
				long file_length;
				data_get_value(&data, "%ld", &file_length);
				printf("%s : %ld\n", filename, file_length);
				char* tmp_filepath = transfer_client_to_server(active_client, filename, file_length);
				set_dragdrop_files(tmp_filepath);
			}
			else if (IS_COMMAND(COMMAND_PING, data))
			{
				printf("PING: %llu\n", (unsigned long long)(time(0) - ping_start));
			}
			else if (IS_COMMAND(COMMAND_NEXT_SCREEN, data))
			{
				int screen_direction;
				int x;
				int y;
				data_get_value(&data, "%d %d %d", &screen_direction, &x, &y);
				y = UNSCALE_Y(y);
				x = UNSCALE_X(x);
				struct client* next_client = client_get_client_in_direction(active_client, screen_direction);
				if (next_client == 0) continue;
				else if (edge_hit_is_dead_corner((struct vec){x, y}, active_client->dead_corners))
					continue;
				else if (next_client == server_client)
				{
					control_state.y = y;
					control_state.x = x;
					control_state.edge_from = screen_direction;
					control_state.state = CONTROL_STATE_SWITCH_TO_MAIN;
					active_client = next_client;
					break;
				}
				else if (next_client)
				{
					control_state.state = CONTROL_STATE_CLIENT;
					send_command(COMMAND_CURSOR_TO, "%d %d", SCALE_X(screen_size.x/2), SCALE_Y(screen_size.y/2));
					active_client = next_client;

					struct vec edge_pos = get_scaled_vec_close_to_edge((struct vec){x, y}, other_edge(screen_direction));
					send_command(COMMAND_CURSOR_TO, "%d %d", edge_pos.x, edge_pos.y);
					break;
				}
			}
		} while ((data = extract_command(0)));
	}
})

CREATE_THREAD(accept_clients, void*, _, {
	(void)_;
	while (1) get_client();
	return 0;
})

static void switch_state_to_main(void)
{
	device_control_enable_input();
	struct vec edge_pos = get_vec_close_to_edge((struct vec){control_state.x, control_state.y}, other_edge(control_state.edge_from));
	device_control_cursor_move_to(edge_pos.x, edge_pos.y);
	control_state.state = CONTROL_STATE_MAIN;
	send_command(COMMAND_PING, "", 0);
	ping_start = time(0);
	printf("time: %ld\n", ping_start);
}

static void switch_to_client_on_edge(int edge_hit)
{
	struct vec pos = device_control_cursor_get();

	struct client* next_client = client_get_client_in_direction(active_client, edge_hit);
	if (next_client == 0 || next_client->active == false)
		return;
	if (edge_hit_is_dead_corner(pos, active_client->dead_corners))
		return;
	active_client = next_client;

	send_command(COMMAND_GO_BY_EDGE_AT, "%d %d", other_edge(edge_hit), get_scaled_pos_on_edge(edge_hit, pos));

//    send_command(COMMAND_UPDATE_CLIPBOARD, "%s\x01", device_control_clipboard_get());

	control_state.state = CONTROL_STATE_CLIENT;
	device_control_disable_input();

	device_control_keyboard_flush();
	device_control_mouse_flush();
}

static void test_edge_hit(void)
{
	struct vec pos = device_control_cursor_get();
	int edge_hit = get_edge_hit(pos);
	if (edge_hit != EDGE_NONE)
		switch_to_client_on_edge(edge_hit);
}

void forward_mouse_input(struct mouse_state mouse_state)
{
	static bool old_mouse_left = false;
	static bool old_mouse_right = false;

	struct vec vel = {mouse_state.x, mouse_state.y};
	send_command(COMMAND_CURSOR_UPDATE, "%d %d",
				(int)((float)vel.x * active_client->mouse_speed),
				(int)((float)vel.y * active_client->mouse_speed));

	device_control_cursor_move_to(screen_size.x / 2, screen_size.y / 2);

	if (mouse_state.scroll) send_command(COMMAND_SCROLL, "%d %d", mouse_state.scroll, active_client->scroll_speed);

	if (mouse_state.left != old_mouse_left)
	{
		if (mouse_state.left) send_command(COMMAND_LEFT_DOWN, "", 0);
		else send_command(COMMAND_LEFT_UP, "", 0);
		old_mouse_left = mouse_state.left;
	}

	if (mouse_state.right != old_mouse_right)
	{
		if (mouse_state.right) send_command(COMMAND_RIGHT_DOWN, "", 0);
		else send_command(COMMAND_RIGHT_UP, "", 0);
		old_mouse_right = mouse_state.right;
	}
}

void forward_keyboard_input(struct key_event key_event)
{
	if (key_event.action == KEY_PRESS)
		send_command(COMMAND_KEYPRESS, "%d", key_event.key);
	else if (key_event.action == KEY_RELEASE)
		send_command(COMMAND_KEYRELEASE, "%d", key_event.key);
}

static void forward_input(void)
{
	struct mouse_state mouse_state = device_control_get_mouse_state();
	if (mouse_state.ready)
		forward_mouse_input(mouse_state);

	struct key_event key_event = device_control_get_keyboard_event();
	if (key_event.ready)
		forward_keyboard_input(key_event);
}

void controller_main(int port)
{
	printf("starting controller on port %d...\n", port);
	active_client = server_client;

	control_state.state = CONTROL_STATE_MAIN;
	server = make_dsocket_tcp_server(port);
	dsocket_tcp_server_bind(&server);
	dsocket_tcp_server_start_listen(&server);

	THREAD controller_receive_thread = MAKE_THREAD();
	THREAD_CALL(&controller_receive_thread, controller_receive, &active_client->sck);

	THREAD accept_clients_thread = MAKE_THREAD();
	THREAD_CALL(&accept_clients_thread, accept_clients, 0);

	while (1)
	{
		SLEEP(REST_TIME);
		if (control_state.state == CONTROL_STATE_CLIENT)
			forward_input();
		else if (control_state.state == CONTROL_STATE_SWITCH_TO_MAIN)
			switch_state_to_main();
		else if (control_state.state == CONTROL_STATE_MAIN)
			test_edge_hit();
		else if (control_state.state == CONTROL_STATE_QUIT)
		{
			THREAD_KILL(&controller_receive_thread);
			THREAD_KILL(&accept_clients_thread);
			return;
		}
	}
}
