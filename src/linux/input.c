#include "input.h"

#ifdef __unix__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void get_param_of(char* buffer_out, char* param, char* find, char* ending)
{
	int data_cap = 1024;
	int data_len = 0;
	char* data = malloc(data_cap);
	char* ptr = 0;

	FILE* fp = fopen("/proc/bus/input/devices", "r");

	char buffer[1024];
	int bytes_read = 0;
	while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0)
	{
		if (data_len + bytes_read + 100 >= data_cap)
		{
			data_cap = data_len + bytes_read + 512;
			data = realloc(data, data_cap);
		}
		strncat(data, buffer, bytes_read);
		data_len += bytes_read;
	}

	fclose(fp);

	ptr = strstr(data, find);

	while (ptr != data)
	{
		if (!strncmp(ptr, "\n\n", 2))
			break;
		ptr--;
	}

	ptr = strstr(ptr, param) + strlen(param);

	ptr = strtok(ptr, ending);

	strcpy(buffer_out, ptr);

	free(data);
}

int get_mouse_event(void)
{
	char buffer[32];
	get_param_of(buffer, "event", "EV=b", " \t\n\r\0");
	return atoi(buffer);
}

void get_mouse_name(char* buffer_out)
{
	get_param_of(buffer_out, "Name=\"", "EV=b", "\"\0");
}

int get_keyboard_event(void)
{
	char buffer[32];
	get_param_of(buffer, "event", "EV=120013", " \t\n\r\0");
	return atoi(buffer);
}

void get_keyboard_name(char* buffer_out)
{
	get_param_of(buffer_out, "Name=\"", "EV=120013", "\"\0");
}

void get_keyboard_input_path(char* buffer_out)
{
	buffer_out[0] = 0;

	int event_num = get_keyboard_event();
	if (event_num == -1)
		return;

	sprintf(buffer_out, "/dev/input/event%d", event_num);
}

void get_mouse_input_path(char* buffer_out)
{
	buffer_out[0] = 0;

	int event_num = get_mouse_event();
	if (event_num == -1)
		return;

	sprintf(buffer_out, "%s%d", "/dev/input/event", event_num);
}

#endif