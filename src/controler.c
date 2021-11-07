#include "controler.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "device_control.h"
#include "commands.h"
#include "utils.h"
#include "ddcSocket.h"
#include "config.h"

#define data_length (4096)

static struct dsocket_tcp_server server;
static int client;

int state = 0;
int gy = 0;

#define send_command(command, format, ...) _send_command("[" command format "]",  __VA_ARGS__)
static void _send_command(char* fmt, ...)
{
	char buffer[1024];

	va_list args;
	va_start(args, fmt);

	vsprintf(buffer, fmt, args);

	va_end(args);

	dsocket_tcp_server_send(server, client, buffer, strlen(buffer));
}


CREATE_THREAD(controler_receive, int, client, {
	while (1)
	{
		char buffer[4096] = {0};
		dsocket_tcp_server_receive(server, client, buffer, sizeof(buffer));
		char* data = extract_command(buffer);
		do
		{
			if (IS_COMMAND(COMMAND_RETURN, data))
			{
				state = 2;
				data_get_value(&data, "%d", &gy);
				gy = UNSCALE_Y(gy);
			}
		} while ((data = extract_command(0)));
	}

});


void controler_init(int port)
{
	server = make_dsocket_tcp_server(port);
	dsocket_tcp_server_bind(&server);
	dsocket_tcp_server_start_listen(&server);
	client = dsocket_tcp_server_listen(&server);

	THREAD_CALL(controler_receive, &client);

	bool old_mouse_left = false;
	bool old_mouse_right = false;

	while (1)
	{
		//usleep(1000);
		if (state != 1)
		{
			if (state == 2)
			{
				device_control_keyboard_enable();
				device_control_cursor_move_to(10, gy);
				state = 0;
				continue;
			}
			struct vec pos = device_control_cursor_get();
			if (pos.x <= 0 && pos.y != 0)
			{
				device_control_keyboard_disable();
				state = 1;
				char data[data_length*2] = {0};
				sprintf(data, "[" COMMAND_CURSOR_TO "%d %d]", SCALE_X(screen_size.x - 5), SCALE_Y(pos.y));
				dsocket_tcp_server_send(server, client, data, strlen(data));
			}
			continue;
		}
		struct mouse_state mouse_state = device_control_get_mouse_state();
		struct key_event key_event = device_control_get_keyboard_event();
		if (!mouse_state.ready && !key_event.ready) continue;
		if (mouse_state.ready)
		{
	
			if (mouse_state.x > 30 || mouse_state.x < -30 ||
				mouse_state.y > 30 || mouse_state.y < -30)
					continue;
	
			struct vec pos = {mouse_state.x, mouse_state.y};
			device_control_cursor_move_to(screen_size.x, screen_size.y / 2);
			send_command(COMMAND_CURSOR_UPDATE, "%d %d", pos.x, pos.y);

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
		if (key_event.ready)
		{
			if (key_event.action == KEY_PRESS)
				send_command(COMMAND_KEYPRESS, "%d", key_event.key);
			else if (key_event.action == KEY_RELEASE)
				send_command(COMMAND_KEYRELEASE, "%d", key_event.key);
		}
	}
}