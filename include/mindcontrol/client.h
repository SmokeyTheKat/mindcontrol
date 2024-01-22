#ifndef __MINDCONTROL_RECEIVER_H__
#define __MINDCONTROL_RECEIVER_H__

#include <stdbool.h>

#include <mindcontrol/vec.h>

#define ITERATE_OVER_CLIENTS(_code) { \
		struct client* client = server_client; \
		struct client* current; \
		struct list seen = make_list(100, struct client*); \
		struct list queue = make_list(100, struct client*); \
		list_push_back(&queue, client, struct client*); \
		list_push_back(&seen, client, struct client*); \
		while (queue.length != 0) \
		{ \
			current = list_first(&queue, struct client*); \
			list_remove(&queue, 0, struct client*); \
			{ _code } \
			for (int j = 0; j < 4; j++) \
			{ \
				if (current->directions[j] && \
					list_index_of(&seen, current->directions[j], struct client*) == -1) \
				{ \
					list_push_back(&queue, current->directions[j], struct client*); \
					list_push_back(&seen, current->directions[j], struct client*); \
				} \
			} \
		} \
	}

struct client;

void receiver_main(char* ip, int port);
void receiver_cleanup(void);

struct client* client_find_by_pos(struct client* client, int x, int y);
struct client* client_find_by_ip(struct client* client, char* ip);
struct client* client_find_by_socket(struct client* client, int sck);
struct client* client_get_client_in_direction(struct client* client, int edge);

extern struct client* server_client;

struct client_info {
	char ip[16];
};

struct client {
	bool active;
	char ip[16];
	struct vec pos;
	int sck;

	float mouse_speed;
	int scroll_speed;

	struct dead_corners {
		int top_left;
		int top_right;
		int bottom_left;
		int bottom_right;
		int size;
	} dead_corners;

	union {
		struct {
			struct client* up;
			struct client* down;
			struct client* left;
			struct client* right;
		};
		struct client* directions[4];
	};
};

#endif
