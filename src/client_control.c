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
#include "list.h"
#include "dragdrop.h"

#define send_command(command, format, ...) _send_command("[" command format "]", __VA_ARGS__)
static void _send_command(char* fmt, ...);
static void interrupt_command(char* data);

static struct dsocket_tcp_client client;

long get_file_length(char* filepath)
{
	FILE* fp = fopen(filepath, "rb");
	if (fp == 0) return 0;
	fseek(fp, 0L, SEEK_END);
	long length = ftell(fp) - 1;
	fclose(fp);
	return length;
}

void transfer_file_to_server(char* filepath)
{
	long file_length = get_file_length(filepath);
	printf("len: %ld\n", file_length);
	send_command(COMMAND_TRANSFER_FILE, "%s\x01 %ld", filepath, file_length);

	FILE* fp = fopen(filepath, "rb");

	if (fp == 0)
		return;

	int byte_count;
	int bytes_read = 0;
	char buffer[4096];
	while (byte_count < file_length && (byte_count = fread(buffer, 1, sizeof(buffer), fp)) > 0)
	{
		if (dsocket_tcp_client_send(client, buffer, byte_count) <= 0)
			break;
		bytes_read += byte_count;
	}

	fclose(fp);
}

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
			int scroll, multiplier;
			data_get_value(&data, "%d %d", &scroll, &multiplier);
			for (int i = 0; i < multiplier; i++)
				device_control_cursor_scroll(scroll);
		} break;
		case COMMAND_VALUE_KEYPRESS:
		{
//            printf("COMMAND_KEYPRESS\n");
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
//            printf("COMMAND_CURSOR_UPDATE\n");
			struct vec pos;
			data_get_value(&data, "%d %d", &pos.x, &pos.y);
			pos.x = (int)(((float)pos.x) * mouse_speed);
			pos.y = (int)(((float)pos.y) * mouse_speed);
//            printf("[%d, %d]\n", pos.x, pos.y);
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
		case COMMAND_VALUE_GO_BY_EDGE_AT:
		{
			int edge;
			int edge_pos;
			data_get_value(&data, "%d %d", &edge, &edge_pos);
			
			struct vec pos = get_unscaled_vec_at_edge_pos(edge, edge_pos);
			device_control_cursor_move_to(pos.x, pos.y);
		} break;
	}
}

void receiver_cleanup(void)
{
	close(client.dscr);
}

void receiver_main(char* ip, int port)
{
RECEIVER_INIT_RETRY:
	printf("connecting to controller on %s:%d\n", ip, port);
	client = make_dsocket_tcp_client(ip, port);
	if (dsocket_tcp_client_connect(&client) != 0)
	{
		close(client.dscr);
		goto RECEIVER_INIT_RETRY;
	}

	while (1)
	{
		char buffer[9024] = {0};
		int bytes_read = dsocket_tcp_client_receive(client, buffer, sizeof(buffer));

		if (bytes_read == 0)
		{
			close(client.dscr);
			goto RECEIVER_INIT_RETRY;
		}
		else if (bytes_read < 0)
			continue;

		printf("%s\n", buffer);

		char* data = extract_command(buffer);
		if (data == 0) continue;

		do interrupt_command(data);
		while ((data = extract_command(0)));

		struct vec pos = device_control_cursor_get();
		int edge_hit = get_edge_hit(pos);
		if (edge_hit != EDGE_NONE)
		{
			struct mouse_state mouse_state = device_control_get_mouse_state();
			if (mouse_state.left)
			{
				struct list* files = get_dragdrop_files();
				char* file = list_first(files, char*);
				transfer_file_to_server(file);
			}
			send_command(COMMAND_NEXT_SCREEN, "%d %d %d", edge_hit, SCALE_X(pos.x), SCALE_Y(pos.y));
		}
		else send_command("OMHI", "%d", 0);
	}
}