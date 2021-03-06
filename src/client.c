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

struct client* client_get_client_in_direction(struct client* client, int edge)
{
	if (edge & EDGE_RIGHT)
		if (client->right) return client->right;

	if (edge & EDGE_LEFT)
		if (client->left) return client->left;

	if (edge & EDGE_BOTTOM)
		if (client->down) return client->down;

	if (edge & EDGE_TOP)
		if (client->up) return client->up;

	return 0;
}

struct client* client_find_by_pos(struct client* client, int x, int y)
{
	struct client* current;
	struct list seen = make_list(100, struct client*);
	struct list queue = make_list(100, struct client*);
	list_push_back(&queue, client, struct client*);
	list_push_back(&seen, client, struct client*);
	while (queue.length != 0)
	{
		current = list_first(&queue, struct client*);
		list_remove(&queue, 0, struct client*);

		if (current->pos.x == x && current->pos.y == y) goto CLIENT_FIND_BY_POS_RETURN;
		
		for (int j = 0; j < 4; j++)
		{
			if (current->directions[j] &&
				list_index_of(&seen, current->directions[j], struct client*) == -1)
			{
				list_push_back(&queue, current->directions[j], struct client*);
				list_push_back(&seen, current->directions[j], struct client*);
			}
		}
	}
	current = 0;
CLIENT_FIND_BY_POS_RETURN:
	free_list(&seen, struct client*);
	free_list(&queue, struct client*);
	return current;
}

struct client* client_find_by_ip(struct client* client, char* ip)
{
	struct client* current;
	struct list seen = make_list(100, struct client*);
	struct list queue = make_list(100, struct client*);
	list_push_back(&queue, client, struct client*);
	list_push_back(&seen, client, struct client*);
	while (queue.length != 0)
	{
		current = list_first(&queue, struct client*);
		list_remove(&queue, 0, struct client*);

		if (!strcmp(current->ip, ip)) goto CLIENT_FIND_BY_POS_RETURN;
		
		for (int j = 0; j < 4; j++)
		{
			if (current->directions[j] &&
				list_index_of(&seen, current->directions[j], struct client*) == -1)
			{
				list_push_back(&queue, current->directions[j], struct client*);
				list_push_back(&seen, current->directions[j], struct client*);
			}
		}
	}
	current = 0;
CLIENT_FIND_BY_POS_RETURN:
	free_list(&seen, struct client*);
	free_list(&queue, struct client*);
	return current;
}

struct client* client_find_by_socket(struct client* client, int sck)
{
	struct client* current;
	struct list seen = make_list(100, struct client*);
	struct list queue = make_list(100, struct client*);
	list_push_back(&queue, client, struct client*);
	list_push_back(&seen, client, struct client*);
	while (queue.length != 0)
	{
		current = list_first(&queue, struct client*);
		list_remove(&queue, 0, struct client*);

		if (current->sck == sck) goto CLIENT_FIND_BY_POS_RETURN;
		
		for (int j = 0; j < 4; j++)
		{
			if (current->directions[j] &&
				list_index_of(&seen, current->directions[j], struct client*) == -1)
			{
				list_push_back(&queue, current->directions[j], struct client*);
				list_push_back(&seen, current->directions[j], struct client*);
			}
		}
	}
	current = 0;
CLIENT_FIND_BY_POS_RETURN:
	free_list(&seen, struct client*);
	free_list(&queue, struct client*);
	return current;
}
