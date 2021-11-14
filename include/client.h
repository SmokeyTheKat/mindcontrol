#ifndef __MINDCONTROL_RECEIVER_H__
#define __MINDCONTROL_RECEIVER_H__

#include <stdbool.h>

#include "vec.h"

struct client;

void receiver_init(char* ip, int port);

struct client
{
	bool active;
	int sck;
	struct vec pos;
};

#endif