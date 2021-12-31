#include "screen.h"

#include "device_control.h"
#include "client.h"
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
			return SCALE_X(screen_size.x - 1);
		case EDGE_LEFT:
			return SCALE_X(1);
	}
	return 0;
}

int get_scaled_y_by_edge(int edge)
{
	switch (edge)
	{
		case EDGE_BOTTOM:
			return SCALE_Y(screen_size.y - 1);
		case EDGE_TOP:
			return SCALE_Y(1);
	}
	return 0;
}

struct vec get_vec_close_to_edge(struct vec pos, int edge)
{
	switch (edge)
	{
		case EDGE_RIGHT:
			return (struct vec){screen_size.x - 1, pos.y};
		case EDGE_LEFT:
			return (struct vec){1, pos.y};
		case EDGE_TOP:
			return (struct vec){pos.x, 1};
		case EDGE_BOTTOM:
			return (struct vec){pos.x, screen_size.y - 1};
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

int get_pos_on_edge(int edge, struct vec pos)
{
	switch (edge)
	{
		case EDGE_TOP:
		case EDGE_BOTTOM:
			return pos.x;
		case EDGE_LEFT:
		case EDGE_RIGHT:
			return pos.y;
	}
	return 0;
}

int get_scaled_pos_on_edge(int edge, struct vec pos)
{
	switch (edge)
	{
		case EDGE_TOP:
		case EDGE_BOTTOM:
			return SCALE_X(pos.x);
		case EDGE_LEFT:
		case EDGE_RIGHT:
			return SCALE_Y(pos.y);
	}
	return 0;
}

struct vec get_vec_at_edge_pos(int edge, int edge_pos)
{
	switch (edge)
	{
		case EDGE_TOP:
			return (struct vec){edge_pos, 1};
		case EDGE_BOTTOM:
			return (struct vec){edge_pos, screen_size.y - 2};
		case EDGE_LEFT:
			return (struct vec){1, edge_pos};
		case EDGE_RIGHT:
			return (struct vec){screen_size.x - 2, edge_pos};
	}
	return (struct vec){0, 0};
}

struct vec get_unscaled_vec_at_edge_pos(int edge, int edge_pos)
{
	switch (edge)
	{
		case EDGE_TOP:
			return (struct vec){UNSCALE_X(edge_pos), 1};
		case EDGE_BOTTOM:
			return (struct vec){UNSCALE_X(edge_pos), screen_size.y - 2};
		case EDGE_LEFT:
			return (struct vec){1, UNSCALE_Y(edge_pos)};
		case EDGE_RIGHT:
			return (struct vec){screen_size.x - 2, UNSCALE_Y(edge_pos)};
	}
	return (struct vec){0, 0};
}

bool edge_hit_is_dead_corner(struct vec pos, struct dead_corners dc)
{
	return (dc.top_left && pos.x <= dc.size - 1 && pos.y <= dc.size - 1) ||
		   (dc.top_right && pos.x >= screen_size.x - dc.size && pos.y <= dc.size - 1) ||
		   (dc.bottom_left && pos.x <= dc.size - 1 && pos.y >= screen_size.y - dc.size) ||
		   (dc.bottom_left && pos.x >= screen_size.x - dc.size && pos.y >= screen_size.y - dc.size);
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