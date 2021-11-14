#include "client.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "ddcSocket.h"
#include "device_control.h"
#include "commands.h"
#include "vec.h"
#include "utils.h"
#include "config.h"
#include "screen.h"

#define send_command(command, format, ...) _send_command("[" command format "]",  __VA_ARGS__)
static void _send_command(char* fmt, ...);
static void interrupt_command(char* data);

static struct dsocket_tcp_client client;

static void _send_command(char* fmt, ...)
{
	char buffer[1024];

	va_list args;
	va_start(args, fmt);

	vsprintf(buffer, fmt, args);

	va_end(args);

	dsocket_tcp_client_send(client, buffer, strlen(buffer));
}

static void interrupt_command(char* data)
{
	switch (COMMAND(data))
	{
		case COMMAND_VALUE_LEFT_UP:
		{
			//printf("COMMAND_LEFT_UP\n");
			device_control_cursor_left_up();
		} break;
		case COMMAND_VALUE_LEFT_DOWN:
		{
			//printf("COMMAND_LEFT_DOWN\n");
			device_control_cursor_left_down();
		} break;
		case COMMAND_VALUE_RIGHT_UP:
		{
			//printf("COMMAND_RIGHT_UP\n");
			device_control_cursor_right_up();
		} break;
		case COMMAND_VALUE_RIGHT_DOWN:
		{
			//printf("COMMAND_RIGHT_DOWN\n");
			device_control_cursor_right_down();
		} break;
		case COMMAND_VALUE_SCROLL:
		{
			//printf("COMMAND_SCROLL\n");
			int scroll;
			data_get_value(&data, "%d", &scroll);
			device_control_cursor_scroll(scroll);
		} break;
		case COMMAND_VALUE_KEYPRESS:
		{
			//printf("COMMAND_KEYPRESS\n");
			int keycode;
			data_get_value(&data, "%d", &keycode);
			device_control_keyboard_send_press(generic_code_to_system_code(keycode));
		} break;
		case COMMAND_VALUE_KEYRELEASE:
		{
			//printf("COMMAND_KEYRELEASE\n");
			int keycode;
			data_get_value(&data, "%d", &keycode);
			device_control_keyboard_send_release(generic_code_to_system_code(keycode));
		} break;
		case COMMAND_VALUE_CURSOR_UPDATE:
		{
			//printf("COMMAND_CURSOR_UPDATE\n");
			struct vec pos;
			data_get_value(&data, "%d %d", &pos.x, &pos.y);
			pos.x = (int)(((float)pos.x) * mouse_speed);
			pos.y = (int)(((float)pos.y) * mouse_speed);
			device_control_cursor_move(pos.x, pos.y);
		} break;
		case COMMAND_VALUE_CURSOR_TO:
		{
			//printf("COMMAND_CURSOR_TO\n");
			struct vec pos;
			data_get_value(&data, "%d %d", &pos.x, &pos.y);
			pos.x = UNSCALE_X(pos.x);
			pos.y = UNSCALE_Y(pos.y);
			//printf("%d %d\n", pos.x, pos.y);
			device_control_cursor_move_to(pos.x, pos.y);
		} break;
		case COMMAND_VALUE_UPDATE_CLIPBOARD:
		{
			char clip_data[8192];
			data_get_string(&data, clip_data);
			device_control_clipboard_set(clip_data);
		} break;
	}
}

void receiver_init(char* ip, int port)
{
	client = make_dsocket_tcp_client(ip, port);
	dsocket_tcp_client_connect(&client);

	while (1)
	{
		char buffer[9024] = {0};
		if (dsocket_tcp_client_receive(client, buffer, sizeof(buffer)) <= 0)
			exit(0);

		char* data = extract_command(buffer);
		if (data == 0) continue;

		do interrupt_command(data);
		while ((data = extract_command(0)));

		struct vec pos = device_control_cursor_get();
		int edge_hit = get_edge_hit(pos);
		if (edge_hit != EDGE_NONE)
		{
			send_command(COMMAND_NEXT_SCREEN, "%d %d %d", edge_hit, SCALE_X(pos.x), SCALE_Y(pos.y));
		}
	}
}