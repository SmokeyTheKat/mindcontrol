#include <mindcontrol/screen.h>

#include <mindcontrol/device_control.h>
#include <mindcontrol/client.h>
#include <mindcontrol/config.h>
#include <mindcontrol/utils.h>

int get_edge_hit(struct vec pos) {
	int edges = 0;
	if (pos.x >= g_screen_size.x - 1) edges |= EDGE_RIGHT;
	if (pos.x <= 0) edges |= EDGE_LEFT;
	if (pos.y >= g_screen_size.y - 1) edges |= EDGE_BOTTOM;
	if (pos.y <= 0) edges |= EDGE_TOP;
	return edges;
}

int get_scaled_x_by_edge(int edge) {
	if (edge & EDGE_RIGHT) {
		return SCALE_X(g_screen_size.x - 1);
	} else if (edge & EDGE_LEFT) {
		return SCALE_X(1);
	} else {
		return 0;
	}
}

int get_scaled_y_by_edge(int edge) {
	if (edge & EDGE_BOTTOM) {
		return SCALE_Y(g_screen_size.y - 1);
	} else if (edge & EDGE_TOP) {
		return SCALE_Y(1);
	} else {
		return 0;
	}
}

struct vec get_vec_close_to_edge(struct vec pos, int edge, int dist) {
	if (edge & EDGE_RIGHT) {
		return (struct vec){ g_screen_size.x - dist - 1, pos.y };
	} else if (edge & EDGE_LEFT) {
		return (struct vec){ dist, pos.y};
	} else if (edge & EDGE_BOTTOM) {
		return (struct vec){ pos.x, g_screen_size.y - dist - 1 };
	} else if (edge & EDGE_TOP) {
		return (struct vec){ pos.x, dist };
	} else {
		return (struct vec){ 0, 0 };
	}
}

struct vec get_scaled_vec_close_to_edge(struct vec pos, int edge, int dist) {
	struct vec new_pos = get_vec_close_to_edge(pos, edge, dist);
	new_pos.x = SCALE_X(new_pos.x);
	new_pos.y = SCALE_Y(new_pos.y);
	return new_pos;
}

int get_pos_on_edge(int edge, struct vec pos) {
	if ((edge & EDGE_LEFT) || (edge & EDGE_RIGHT)) {
		return pos.y;
	} else if ((edge & EDGE_TOP) || (edge & EDGE_BOTTOM)) {
		return pos.x;
	} else {
		return 0;
	}
}

int get_scaled_pos_on_edge(int edge, struct vec pos) {
	if ((edge & EDGE_LEFT) || (edge & EDGE_RIGHT)) {
		return SCALE_Y(pos.y);
	} else if ((edge & EDGE_TOP) || (edge & EDGE_BOTTOM)) {
		return SCALE_X(pos.x);
	} else {
		return 0;
	}
}

struct vec get_vec_at_edge_pos(int edge, int edge_pos) {
	if (edge & EDGE_RIGHT) {
		return (struct vec){ g_screen_size.x - 2, edge_pos };
	} else if (edge & EDGE_LEFT) {
		return (struct vec){ 1, edge_pos };
	} else if (edge & EDGE_BOTTOM) {
		return (struct vec){ edge_pos, g_screen_size.y - 2 };
	} else if (edge & EDGE_TOP) {
		return (struct vec){ edge_pos, 1 };
	} else {
		return (struct vec){ 0, 0 };
	}
}

struct vec get_unscaled_vec_at_edge_pos(int edge, int edge_pos) {
	if (edge & EDGE_RIGHT) {
		return (struct vec){ g_screen_size.x - 3, UNSCALE_Y(edge_pos) };
	} else if (edge & EDGE_LEFT) {
		return (struct vec){ 2, UNSCALE_Y(edge_pos) };
	} else if (edge & EDGE_BOTTOM) {
		return (struct vec){ UNSCALE_X(edge_pos), g_screen_size.y - 3 };
	} else if (edge & EDGE_TOP) {
		return (struct vec){ UNSCALE_X(edge_pos), 2 };
	} else {
		return (struct vec){ 0, 0 };
	}
}

bool edge_hit_is_dead_corner(struct vec pos, struct dead_corners dc) {
	return (
		(dc.top_left && pos.x <= dc.size - 1 && pos.y <= dc.size - 1) ||
		(dc.top_right && pos.x >= g_screen_size.x - dc.size && pos.y <= dc.size - 1) ||
		(dc.bottom_left && pos.x <= dc.size - 1 && pos.y >= g_screen_size.y - dc.size) ||
		(dc.bottom_right && pos.x >= g_screen_size.x - dc.size && pos.y >= g_screen_size.y - dc.size)
	);
}

struct vec screen_unscale_vec(struct vec vec) {
	return (struct vec){ UNSCALE_X(vec.x), UNSCALE_Y(vec.y) };
}

struct vec screen_scale_vec(struct vec vec) {
	return (struct vec){ SCALE_X(vec.x), SCALE_Y(vec.y) };
}

int other_edge(int edge) {
	if (edge & EDGE_RIGHT) {
		return edge ^ (EDGE_RIGHT | EDGE_LEFT);
	} else if (edge & EDGE_LEFT) {
		return edge ^ (EDGE_LEFT | EDGE_RIGHT);
	} else if (edge & EDGE_BOTTOM) {
		return edge ^ (EDGE_BOTTOM | EDGE_TOP);
	} else if (edge & EDGE_TOP) {
		return edge ^ (EDGE_TOP | EDGE_BOTTOM);
	} else {
		return EDGE_NONE;
	}
}
