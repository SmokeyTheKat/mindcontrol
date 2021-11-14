#ifndef __MINDCONTROL_SCREEN_H__
#define __MINDCONTROL_SCREEN_H__

#include "vec.h"

#define EDGE_NONE 0
#define EDGE_RIGHT 1
#define EDGE_LEFT 2
#define EDGE_BOTTOM 3
#define EDGE_TOP 4

int get_edge_hit(struct vec pos);
int get_scaled_x_by_edge(int edge);
int get_scaled_y_by_edge(int edge);
struct vec get_vec_close_to_edge(struct vec pos, int edge);
struct vec get_scaled_vec_close_to_edge(struct vec pos, int edge);
int other_edge(int edge);


#endif