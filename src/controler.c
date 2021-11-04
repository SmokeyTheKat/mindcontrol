#include "controler.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/poll.h>
#include <sys/select.h>

#include "device_control.h"
#include "commands.h"
#include "utils.h"
#include "ddcSocket.h"

#define data_length (4096)

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define SCREEN_SCALE 10000

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

#ifdef __unix__

void* controler_receive(void* vclient)
{
	int client = *(int*)vclient;
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
}

#endif

#ifdef __WIN64

#include <windows.h>

DWORD WINAPI controler_receive(LPVOID lp_client)
{
	int client = *(int*)lp_client;
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
}

#endif

void controler_init(int port)
{
	server = make_dsocket_tcp_server(port);
	dsocket_tcp_server_bind(&server);
	dsocket_tcp_server_start_listen(&server);
	client = dsocket_tcp_server_listen(&server);

#ifdef __unix__
	pthread_t controler_thread;
	pthread_create(&controler_thread, 0, controler_receive, &client);
#endif

#ifdef __WIN64
	DWORD controler_thread;
	HANDLE controler_handle = CreateThread(0, 0, controler_receive, &client;, 0, &controler_thread);
#endif

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
				sprintf(data, "[" COMMAND_CURSOR_TO "%d %d]", SCALE_X(SCREEN_WIDTH - 5), SCALE_Y(pos.y));
				dsocket_tcp_server_send(server, client, data, strlen(data));
			}
			continue;
		}
		struct mouse_state mouse_state = device_control_get_mouse_state();
		struct key_event key_event = device_control_get_keyboard_event();
		if (!mouse_state.ready && !key_event.ready) continue;
		if (mouse_state.ready)
		{
	
			if (mouse_state.x > 90 || mouse_state.x < -90 ||
				mouse_state.y > 90 || mouse_state.y < -90)
					continue;
	
			struct vec pos = {mouse_state.x, mouse_state.y};
			device_control_cursor_move_to(SCREEN_WIDTH/2, SCREEN_HEIGHT/2);
			send_command(COMMAND_CURSOR_UPDATE, "%d %d", pos.x, pos.y);

			if (mouse_state.scroll) send_command(COMMAND_SCROLL, "%d", mouse_state.scroll);
	
			if (mouse_state.left) send_command(COMMAND_LEFT_DOWN, "", 0);
			else send_command(COMMAND_LEFT_UP, "", 0);
	
			if (mouse_state.right) send_command(COMMAND_RIGHT_DOWN, "", 0);
			else send_command(COMMAND_RIGHT_UP, "", 0);
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