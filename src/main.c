#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vec.h"
#include "device_control.h"
#include "controller.h"
#include "client.h"
#include "config.h"
#include "gui.h"
#include "dragdrop.h"

#ifdef _WIN64
#include "windows/wininfo.h"
HINSTANCE hinstance;
int WINAPI WinMain(HINSTANCE _hinstance,
				   HINSTANCE hprev_instance,
				   LPSTR cmd_line,
				   int cmd_show)
{
	int argc;
	char** argv = CommandLineToArgvW(cmd_line, &argc);
	hinstance = _hinstance;
#else
int main(int argc, char** argv)
{
#endif
	read_args(argc, argv);
	device_control_init();
	if (user_type == 'c') controller_main(server_port);
	else if (user_type == 'r') receiver_main(server_ip, server_port);
	else gui_main();
	return 0;
}