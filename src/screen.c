#include "screen.h"

#include "device_control.h"
#include "config.h"
#include "utils.h"

int get_edge_hit(struct vec pos)
{
	if (pos.x >= screen_size.x - 1)
		return EDGE_RIGHT;
	if (pos.x <= 0)
		return EDGE_LEFT;
	if (pos.y >= screen_size.y - 1)
		return EDGE_BOTTOM;
	if (pos.y <= 0)
		return EDGE_TOP;
	return EDGE_NONE;
}

int get_scaled_x_by_edge(int edge)
{
	switch (edge)
	{
		case EDGE_RIGHT:
			return SCALE_X(screen_size.x - 5);
		case EDGE_LEFT:
			return SCALE_X(5);
	}
	return 0;
}

int get_scaled_y_by_edge(int edge)
{
	switch (edge)
	{
		case EDGE_BOTTOM:
			return SCALE_Y(screen_size.y - 5);
		case EDGE_TOP:
			return SCALE_Y(5);
	}
	return 0;
}

struct vec get_vec_close_to_edge(struct vec pos, int edge)
{
	switch (edge)
	{
		case EDGE_RIGHT:
			return (struct vec){screen_size.x - 6, pos.y};
		case EDGE_LEFT:
			return (struct vec){5, pos.y};
		case EDGE_TOP:
			return (struct vec){pos.x, 5};
		case EDGE_BOTTOM:
			return (struct vec){pos.x, screen_size.y - 6};
	}
	return (struct vec){0, 0};
}

struct vec get_scaled_vec_close_to_edge(struct vec pos, int edge)
{
	struct vec new_pos = get_vec_close_to_edge(pos, edge);
	new_pos.x = SCALE_X(new_pos.x);
	new_pos.y = SCALE_Y(new_pos.y);
	return new_pos;
}

int other_edge(int edge)
{
	switch (edge)
	{
		case EDGE_RIGHT:
			return EDGE_LEFT;
		case EDGE_LEFT:
			return EDGE_RIGHT;
		case EDGE_TOP:
			return EDGE_BOTTOM;
		case EDGE_BOTTOM:
			return EDGE_TOP;
		default: return EDGE_NONE;
	}
}