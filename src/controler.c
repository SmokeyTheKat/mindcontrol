#include "controler.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "device_control.h"
#include "commands.h"
#include "utils.h"
#include "ddcSocket.h"
#include "config.h"
#include "screen.h"
#include "client.h"
#include "state.h"

#define MAX_VEL ((struct vec){30, 30})

#define send_command(command, format, ...) _send_command("[" command format "]",  __VA_ARGS__)
static void _send_command(char* fmt, ...);
static void switch_state_to_main(void);
static void switch_to_client_on_edge(int edge_hit);
static void handel_local_control(void);
static void forward_input(void);

static struct dsocket_tcp_server server;

static struct client c_client = (struct client){
	.active=false,
	.sck=-1,
	.ip={"192.168.1.41"},
	.left=0,
	.right=0,
	.up=0,
	.down=0,
};
static struct client server_client = (struct client){
	.active=true,
	.sck=-1,
	.ip={0},
	.left=0,
	.right=0,
	.up=0,
	.down=0,
};
static struct client* active_client = &server_client;

static data_state(int x; int y; int edge_from) control_state;

struct client* get_client_by_ip(struct client* client, char* ip)
{
	if (client == 0) client = &server_client;
	if (!strcmp(ip, client->ip)) return client;
	if (client->right)
	{
		struct client* new_client = get_client_by_ip(client->right, ip);
		if (new_client) return new_client;
	}
	if (client->left)
	{
		struct client* new_client = get_client_by_ip(client->left, ip);
		if (new_client) return new_client;
	}
	if (client->up)
	{
		struct client* new_client = get_client_by_ip(client->up, ip);
		if (new_client) return new_client;
	}
	if (client->down)
	{
		struct client* new_client = get_client_by_ip(client->down, ip);
		if (new_client) return new_client;
	}
	return 0;
}

struct client* get_client(void)
{
	int sck = dsocket_tcp_server_listen(&server);
	char* ip = inet_ntoa(server.server.sin_addr);
	printf("%s\n", ip);
	struct client* client = get_client_by_ip(0, ip);
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

struct client* client_get_client_in_direction(struct client* client, int direction)
{
	switch (direction)
	{
		case EDGE_RIGHT:
			if (client->right) return client->right;
			break;
		case EDGE_LEFT:
			if (client->left) return client->left;
			break;
		case EDGE_BOTTOM:
			if (client->down) return client->down;
			break;
		case EDGE_TOP:
			if (client->up) return client->up;
			break;
	}
	return 0;
}

CREATE_THREAD(controler_receive, void*, _, {
	(void)_;
	while (1)
	{
		if (active_client->sck == -1 || active_client->active == false)
		{
			SLEEP(REST_TIME);
			continue;
		}
		char buffer[4096] = {0};
		dsocket_tcp_server_receive(server, active_client->sck, buffer, sizeof(buffer));
		char* data = extract_command(buffer);
		do
		{
			if (IS_COMMAND(COMMAND_NEXT_SCREEN, data))
			{
				int screen_direction;
				int x;
				int y;
				data_get_value(&data, "%d %d %d", &screen_direction, &x, &y);
				y = UNSCALE_Y(y);
				x = UNSCALE_X(x);
				struct client* next_client = client_get_client_in_direction(active_client, screen_direction);
				if (next_client == 0) continue;
				else if (next_client == &server_client)
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

					struct vec edge_pos = get_scaled_vec_close_to_edge((struct vec){x, y}, screen_direction);
					send_command(COMMAND_CURSOR_TO, "%d %d", edge_pos.x, edge_pos.y);
//                    send_command(COMMAND_CURSOR_TO, "%d %d",
//                                 get_scaled_x_by_edge(other_edge(screen_direction)),
//                                 SCALE_Y(y));
					break;
				}
			}
		} while ((data = extract_command(0)));
	}
});

CREATE_THREAD(accept_clients, void*, _, {
	(void)_;
	get_client();
	return 0;
});

static void switch_state_to_main(void)
{
	device_control_keyboard_enable();
	struct vec edge_pos = get_vec_close_to_edge((struct vec){control_state.x, control_state.y}, other_edge(control_state.edge_from));
	device_control_cursor_move_to(edge_pos.x, edge_pos.y);
	control_state.state = CONTROL_STATE_MAIN;
}

static void switch_to_client_on_edge(int edge_hit)
{
	struct vec pos = device_control_cursor_get();

	struct client* next_client = client_get_client_in_direction(active_client, edge_hit);
	if (next_client == 0 || next_client->active == false) return;
	active_client = next_client;

	device_control_cursor_move_to(screen_size.x / 2, screen_size.y / 2);

	struct vec edge_pos = get_scaled_vec_close_to_edge(pos, other_edge(edge_hit));
	send_command(COMMAND_CURSOR_TO, "%d %d", edge_pos.x, edge_pos.y);

	send_command(COMMAND_UPDATE_CLIPBOARD, "%s\x01", device_control_clipboard_get());

	control_state.state = CONTROL_STATE_CLIENT;
	device_control_keyboard_disable();
	device_control_get_mouse_state();
}

static void handel_local_control(void)
{
	if (control_state.state == CONTROL_STATE_SWITCH_TO_MAIN)
		switch_state_to_main();
	else 
	{
		struct vec pos = device_control_cursor_get();
		int edge_hit = get_edge_hit(pos);
		if (edge_hit != EDGE_NONE)
			switch_to_client_on_edge(edge_hit);
	}
}

void forward_mouse_input(struct mouse_state mouse_state)
{
	static bool old_mouse_left = false;
	static bool old_mouse_right = false;

	device_control_cursor_move_to(screen_size.x / 2, screen_size.y / 2);

	struct vec vel = {mouse_state.x, mouse_state.y};
	if (vec_is_above_limit(vel, MAX_VEL)) return;
	send_command(COMMAND_CURSOR_UPDATE, "%d %d", vel.x, vel.y);

	if (mouse_state.scroll) send_command(COMMAND_SCROLL, "%d", mouse_state.scroll);

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

void controler_init(int port)
{
	server_client.left = &c_client;
	c_client.right = &server_client;

	control_state.state = CONTROL_STATE_MAIN;
	server = make_dsocket_tcp_server(port);
	dsocket_tcp_server_bind(&server);
	dsocket_tcp_server_start_listen(&server);

	THREAD_CALL(controler_receive, &active_client->sck);
	THREAD_CALL(accept_clients, 0);

	while (1)
	{
		SLEEP(REST_TIME);
		if (control_state.state == CONTROL_STATE_CLIENT)
			forward_input();
		else handel_local_control();
	}
}