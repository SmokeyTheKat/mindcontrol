#include <stdio.h>

#include "vec.h"
#include "device_control.h"
#include "controler.h"
#include "receiver.h"





int main(int argc, char** argv)
{
	if (argc == 1) return 0;
	device_control_init();
	if (argv[1][0] == 't')
	{
		return 0;
	}
	if (argv[1][0] == 'c') controler_init(1168);
	if (argv[1][0] == 'r') receiver_init("192.168.1.20", 1168);
	return 0;
}