#ifndef __MINDCONTROL_VEC_H__
#define __MINDCONTROL_VEC_H__

#include <stdbool.h>

struct vec {
	int x, y;
};

bool vec_compare(struct vec a, struct vec b);
struct vec vec_add(struct vec a, struct vec b);
struct vec vec_sub(struct vec a, struct vec b);
struct vec vec_mul(struct vec a, struct vec b);
struct vec vec_div(struct vec a, struct vec b);
struct vec vec_add_const(struct vec a, int v);
struct vec vec_sub_const(struct vec a, int v);
struct vec vec_mul_const(struct vec a, int v);
struct vec vec_div_const(struct vec a, int v);
struct vec vec_add_float(struct vec a, float v);
struct vec vec_sub_float(struct vec a, float v);
struct vec vec_mul_float(struct vec a, float v);
struct vec vec_div_float(struct vec a, float v);
struct vec vec_limit(struct vec vec, struct vec limit);
bool vec_is_above_limit(struct vec vec, struct vec limit);

#endif
