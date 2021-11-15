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
	char ip[16];
	struct client* left;
	struct client* right;
	struct client* up;
	struct client* down;
};

#endif