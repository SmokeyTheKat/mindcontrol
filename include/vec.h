#ifndef __MINDCONTROL_VEC_H__
#define __MINDCONTROL_VEC_H__

#include <stdbool.h>

struct vec
{
	int x, y;
};

bool vec_compare(struct vec a, struct vec b);
struct vec vec_sub(struct vec a, struct vec b);
struct vec vec_limit(struct vec vec, struct vec limit);

#endif