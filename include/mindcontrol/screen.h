#ifndef __MINDCONTROL_SCREEN_H__
#define __MINDCONTROL_SCREEN_H__

#include <mindcontrol/vec.h>
#include <mindcontrol/client.h>

#define EDGE_NONE 0
#define EDGE_RIGHT (1 << 1)
#define EDGE_LEFT (1 << 2)
#define EDGE_BOTTOM (1 << 3)
#define EDGE_TOP (1 << 4)

int get_edge_hit(struct vec pos);
int get_scaled_x_by_edge(int edge);
int get_scaled_y_by_edge(int edge);
struct vec get_vec_close_to_edge(struct vec pos, int edge, int dist);
struct vec get_scaled_vec_close_to_edge(struct vec pos, int edge, int dist);
int get_scaled_pos_on_edge(int edge, struct vec pos);
struct vec get_vec_at_edge_pos(int edge, int edge_pos);
struct vec get_unscaled_vec_at_edge_pos(int edge, int edge_pos);
bool edge_hit_is_dead_corner(struct vec pos, struct dead_corners dc);
struct vec screen_scale_vec(struct vec vec);
struct vec screen_unscale_vec(struct vec vec);
int other_edge(int edge);


#endif
