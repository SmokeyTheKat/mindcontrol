#include "receiver.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "ddcSocket.h"
#include "device_control.h"
#include "commands.h"
#include "vec.h"
#include "utils.h"
#include "config.h"

static struct dsocket_tcp_client client;

#define send_command(command, format, ...) _send_command("[" command format "]",  __VA_ARGS__)
static void _send_command(char* fmt, ...)
{
	char buffer[1024];

	va_list args;
	va_start(args, fmt);

	vsprintf(buffer, fmt, args);

	va_end(args);

	dsocket_tcp_client_send(client, buffer, strlen(buffer));
}

void receiver_init(char* ip, int port)
{
	client = make_dsocket_tcp_client(ip, port);
	dsocket_tcp_client_connect(&client);
	while (1)
	{
		char buffer[1024] = {0};
		dsocket_tcp_client_receive(client, buffer, sizeof(buffer));

		char* data = extract_command(buffer);
		if (data == 0) continue;
		//printf("%s\n", data);
		do
		{
			if (IS_COMMAND(COMMAND_LEFT_UP, data))
			{
				//printf("COMMAND_LEFT_UP\n");
				device_control_cursor_left_up();
			}
			else if (IS_COMMAND(COMMAND_LEFT_DOWN, data))
			{
				//printf("COMMAND_LEFT_DOWN\n");
				device_control_cursor_left_down();
			}
	
			if (IS_COMMAND(COMMAND_RIGHT_UP, data))
			{
				//printf("COMMAND_RIGHT_UP\n");
				device_control_cursor_right_up();
			}
			else if (IS_COMMAND(COMMAND_RIGHT_DOWN, data))
			{
				//printf("COMMAND_RIGHT_DOWN\n");
				device_control_cursor_right_down();
			}
	
			if (IS_COMMAND(COMMAND_SCROLL, data))
			{
				//printf("COMMAND_SCROLL\n");
				int scroll;
				data_get_value(&data, "%d", &scroll);
				device_control_cursor_scroll(scroll);
			}
	
			if (IS_COMMAND(COMMAND_KEYPRESS, data))
			{
				//printf("COMMAND_KEYPRESS\n");
				int keycode;
				data_get_value(&data, "%d", &keycode);
				device_control_keyboard_send_press(generic_code_to_system_code(keycode));
			}
			if (IS_COMMAND(COMMAND_KEYRELEASE, data))
			{
				//printf("COMMAND_KEYRELEASE\n");
				int keycode;
				data_get_value(&data, "%d", &keycode);
				device_control_keyboard_send_release(generic_code_to_system_code(keycode));
			}
	
			if (IS_COMMAND(COMMAND_CURSOR_UPDATE, data))
			{
				//printf("COMMAND_CURSOR_UPDATE\n");
				struct vec pos;
				data_get_value(&data, "%d %d", &pos.x, &pos.y);
				pos.x = (int)(((float)pos.x) * mouse_speed);
				pos.y = (int)(((float)pos.y) * mouse_speed);
				device_control_cursor_move(pos.x, pos.y);
			}
			if (IS_COMMAND(COMMAND_CURSOR_TO, data))
			{
				//printf("COMMAND_CURSOR_TO\n");
				struct vec pos;
				data_get_value(&data, "%d %d", &pos.x, &pos.y);
				pos.x = UNSCALE_X(pos.x);
				pos.y = UNSCALE_Y(pos.y);
				//printf("%d %d\n", pos.x, pos.y);
				device_control_cursor_move_to(pos.x, pos.y);
			}
		} while ((data = extract_command(0)));

		struct vec pos = device_control_cursor_get();
		if (pos.x >= screen_size.x - 1)
		{
			send_command(COMMAND_RETURN, "%d", SCALE_Y(pos.y));
			device_control_cursor_move_to(screen_size.x / 2, screen_size.y / 2);
		}
	}
}