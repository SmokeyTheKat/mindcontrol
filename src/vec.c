#include "vec.h"

bool vec_compare(struct vec a, struct vec b)
{
	return a.x == b.x && a.y == b.y;
}

struct vec vec_sub(struct vec a, struct vec b)
{
	return (struct vec){a.x - b.x, a.y - b.y};
}