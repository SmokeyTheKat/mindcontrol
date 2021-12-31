#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vec.h"
#include "device_control.h"
#include "controller.h"
#include "client.h"
#include "config.h"
#include "gui.h"

int main(int argc, char** argv)
{
	if (argc == 1) return 0;
	read_args(argc, argv);
	device_control_init();
	if (user_type == 'c') controller_main(server_port);
	if (user_type == 'r') receiver_main(server_ip, server_port);
	gui_main();
	return 0;
}