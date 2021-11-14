#include "vec.h"

#include "utils.h"

bool vec_compare(struct vec a, struct vec b)
{
	return a.x == b.x && a.y == b.y;
}

struct vec vec_sub(struct vec a, struct vec b)
{
	return (struct vec){a.x - b.x, a.y - b.y};
}

struct vec vec_limit(struct vec vec, struct vec limit)
{
	struct vec out;
	if (ABS(vec.x) < ABS(limit.x))
		out.x = vec.x;
	else out.x = limit.x;
	if (ABS(vec.y) < ABS(limit.y))
		out.y = vec.y;
	else out.y = limit.y;
	return out;
}