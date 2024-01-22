#include <mindcontrol/vec.h>

#include <mindcontrol/utils.h>

bool vec_compare(struct vec a, struct vec b) {
	return (a.x == b.x) && (a.y == b.y);
}

struct vec vec_add(struct vec a, struct vec b) {
	return (struct vec){ a.x + b.x, a.y + b.y };
}

struct vec vec_sub(struct vec a, struct vec b) {
	return (struct vec){ a.x - b.x, a.y - b.y };
}

struct vec vec_mul(struct vec a, struct vec b) {
	return (struct vec){ a.x * b.x, a.y * b.y };
}

struct vec vec_div(struct vec a, struct vec b) {
	return (struct vec){ a.x / b.x, a.y / b.y };
}

struct vec vec_add_const(struct vec a, int v) {
	return (struct vec){ a.x + v, a.y + v };
}

struct vec vec_sub_const(struct vec a, int v) {
	return (struct vec){ a.x - v, a.y - v };
}

struct vec vec_mul_const(struct vec a, int v) {
	return (struct vec){ a.x * v, a.y * v };
}

struct vec vec_div_const(struct vec a, int v) {
	return (struct vec){ a.x / v, a.y / v };
}

struct vec vec_add_float(struct vec a, float v) {
	return (struct vec){ a.x + v, a.y + v };
}

struct vec vec_sub_float(struct vec a, float v) {
	return (struct vec){ a.x - v, a.y - v };
}

struct vec vec_mul_float(struct vec a, float v) {
	return (struct vec){ a.x * v, a.y * v };
}

struct vec vec_div_float(struct vec a, float v) {
	return (struct vec){ a.x / v, a.y / v };
}

struct vec vec_limit(struct vec vec, struct vec limit) {
	struct vec out;
	out.x = ABS(vec.x) < ABS(limit.x) ? vec.x : limit.x;
	out.y = ABS(vec.y) < ABS(limit.y) ? vec.y : limit.y;
	return out;
}

bool vec_is_above_limit(struct vec vec, struct vec limit) {
	return (ABS(vec.x) > ABS(limit.x)) || (ABS(vec.y) > ABS(limit.y));
}
