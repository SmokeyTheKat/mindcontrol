#include "receiver.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>


#include "ddcSocket.h"
#include "device_control.h"
#include "commands.h"
#include "vec.h"
#include "utils.h"

#define SCREEN_WIDTH 1366
#define SCREEN_HEIGHT 768
#define SCREEN_SCALE 10000

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
		do
		{
			if (IS_COMMAND(COMMAND_LEFT_UP, data))
			{
				device_control_cursor_left_up();
			}
			else if (IS_COMMAND(COMMAND_LEFT_DOWN, data))
			{
				device_control_cursor_left_down();
			}
	
			if (IS_COMMAND(COMMAND_RIGHT_UP, data))
			{
				device_control_cursor_right_up();
			}
			else if (IS_COMMAND(COMMAND_RIGHT_DOWN, data))
			{
				device_control_cursor_right_down();
			}
	
			if (IS_COMMAND(COMMAND_SCROLL, data))
			{
				int scroll;
				data_get_value(&data, "%d", &scroll);
				device_control_cursor_scroll(scroll);
			}
	
			if (IS_COMMAND(COMMAND_KEYPRESS, data))
			{
				int keycode;
				data_get_value(&data, "%d", &keycode);
				device_control_keyboard_send_press(keycode);
			}
			if (IS_COMMAND(COMMAND_KEYRELEASE, data))
			{
				int keycode;
				data_get_value(&data, "%d", &keycode);
				device_control_keyboard_send_release(keycode);
			}
	
			if (IS_COMMAND(COMMAND_CURSOR_UPDATE, data))
			{
				struct vec pos;
				data_get_value(&data, "%d %d", &pos.x, &pos.y);
				pos.x = (int)(((float)pos.x) * 5.5);
				pos.y = (int)(((float)pos.y) * 5.5);
				device_control_cursor_move(pos.x, pos.y);
			}
			if (IS_COMMAND(COMMAND_CURSOR_TO, data))
			{
				struct vec pos;
				data_get_value(&data, "%d %d", &pos.x, &pos.y);
				pos.x = UNSCALE_X(pos.x);
				pos.y = UNSCALE_Y(pos.y);
				printf("%d %d\n", pos.x, pos.y);
				device_control_cursor_move_to(pos.x, pos.y);
			}
		} while ((data = extract_command(0)));
		
		struct vec pos = device_control_cursor_get();
		if (pos.x >= SCREEN_WIDTH - 1)
		{
			send_command(COMMAND_RETURN, "%d", SCALE_Y(pos.y));
			device_control_cursor_move_to(SCREEN_WIDTH/2, SCREEN_HEIGHT/2);
		}
	}
}