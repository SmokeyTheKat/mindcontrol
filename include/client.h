#ifndef __MINDCONTROL_RECEIVER_H__
#define __MINDCONTROL_RECEIVER_H__

#include <stdbool.h>

#include "vec.h"

struct client;

void receiver_init(char* ip, int port);

struct client* client_find_by_pos(struct client* client, int x, int y);
struct client* client_find_by_ip(struct client* client, char* ip);
struct client* client_find_by_socket(struct client* client, int sck);

struct client
{
	bool active;
	char ip[16];
	struct vec pos;
	int sck;
	union
	{
		struct
		{
			struct client* up;
			struct client* down;
			struct client* left;
			struct client* right;
		};
		struct client* directions[4];
	};
};

#endif