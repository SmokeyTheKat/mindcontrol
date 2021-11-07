#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __WIN64
#include <windows.h>
#endif

#include "vec.h"
#include "device_control.h"
#include "controler.h"
#include "receiver.h"
#include "config.h"

int main(int argc, char** argv)
{
	if (argc == 1) return 0;
	read_args(argc, argv);
	device_control_init();
	if (user_type == 't')
	{
		device_control_cursor_left_down();
#ifdef __WIN64
		Sleep(10000);
#endif
		device_control_cursor_left_up();
		return 0;
	}
	if (user_type == 'c') controler_init(server_port);
	if (user_type == 'r') receiver_init(server_ip, server_port);
	return 0;
}