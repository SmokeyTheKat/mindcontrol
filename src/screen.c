#include "screen.h"

#include "device_control.h"
#include "client.h"
#include "config.h"
#include "utils.h"

int get_edge_hit(struct vec pos)
{
	int edges = 0;
	if (pos.x >= screen_size.x - 1)
		edges |= EDGE_RIGHT;
	if (pos.x <= 0)
		edges |= EDGE_LEFT;
	if (pos.y >= screen_size.y - 1)
		edges |= EDGE_BOTTOM;
	if (pos.y <= 0)
		edges |= EDGE_TOP;
	return edges;
}

int get_scaled_x_by_edge(int edge)
{
	if (edge & EDGE_RIGHT)
		return SCALE_X(screen_size.x - 1);
	if (edge & EDGE_LEFT)
		return SCALE_X(1);
	return 0;
}

int get_scaled_y_by_edge(int edge)
{
	if (edge & EDGE_BOTTOM)
		return SCALE_Y(screen_size.y - 1);
	if (edge & EDGE_TOP)
		return SCALE_Y(1);
	return 0;
}

struct vec get_vec_close_to_edge(struct vec pos, int edge)
{
	if (edge & EDGE_RIGHT)
		return (struct vec){screen_size.x - 1, pos.y};
	if (edge & EDGE_LEFT)
		return (struct vec){1, pos.y};
	if (edge & EDGE_BOTTOM)
		return (struct vec){pos.x, screen_size.y - 1};
	if (edge & EDGE_TOP)
		return (struct vec){pos.x, 1};
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
	if ((edge & EDGE_LEFT) || (edge & EDGE_RIGHT))
		return pos.y;
	if ((edge & EDGE_TOP) || (edge & EDGE_BOTTOM))
		return pos.x;
	return 0;
}

int get_scaled_pos_on_edge(int edge, struct vec pos)
{
	if ((edge & EDGE_LEFT) || (edge & EDGE_RIGHT))
		return SCALE_Y(pos.y);
	if ((edge & EDGE_TOP) || (edge & EDGE_BOTTOM))
		return SCALE_X(pos.x);
	return 0;
}

struct vec get_vec_at_edge_pos(int edge, int edge_pos)
{
	if (edge & EDGE_RIGHT)
		return (struct vec){screen_size.x - 2, edge_pos};
	if (edge & EDGE_LEFT)
		return (struct vec){1, edge_pos};
	if (edge & EDGE_BOTTOM)
		return (struct vec){edge_pos, screen_size.y - 2};
	if (edge & EDGE_TOP)
		return (struct vec){edge_pos, 1};
	return (struct vec){0, 0};
}

struct vec get_unscaled_vec_at_edge_pos(int edge, int edge_pos)
{
	if (edge & EDGE_RIGHT)
		return (struct vec){screen_size.x - 2, UNSCALE_Y(edge_pos)};
	if (edge & EDGE_LEFT)
		return (struct vec){1, UNSCALE_Y(edge_pos)};
	if (edge & EDGE_BOTTOM)
		return (struct vec){UNSCALE_X(edge_pos), screen_size.y - 2};
	if (edge & EDGE_TOP)
		return (struct vec){UNSCALE_X(edge_pos), 1};
	return (struct vec){0, 0};
}

bool edge_hit_is_dead_corner(struct vec pos, struct dead_corners dc)
{
	return (dc.top_left && pos.x <= dc.size - 1 && pos.y <= dc.size - 1) ||
		   (dc.top_right && pos.x >= screen_size.x - dc.size && pos.y <= dc.size - 1) ||
		   (dc.bottom_left && pos.x <= dc.size - 1 && pos.y >= screen_size.y - dc.size) ||
		   (dc.bottom_right && pos.x >= screen_size.x - dc.size && pos.y >= screen_size.y - dc.size);
}

int other_edge(int edge)
{
	if (edge & EDGE_RIGHT)
		return edge ^ (EDGE_RIGHT | EDGE_LEFT);
	if (edge & EDGE_LEFT)
		return edge ^ (EDGE_LEFT | EDGE_RIGHT);
	if (edge & EDGE_BOTTOM)
		return edge ^ (EDGE_BOTTOM | EDGE_TOP);
	if (edge & EDGE_TOP)
		return edge ^ (EDGE_TOP | EDGE_BOTTOM);
	return EDGE_NONE;
}