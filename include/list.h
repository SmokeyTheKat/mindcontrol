#ifndef __LIST_H__
#define __LIST_H__

#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>

#define LIST_EXPAND_SIZE 4

struct list
{
	char* data;
	intmax_t length;
	intmax_t capacity;
};

intmax_t __list_index_of(struct list* list, void* find, intmax_t type_size);

#define make_list(cap, type) (struct list){ \
		.data=malloc(sizeof(type)*cap), \
		.length=0, \
		.capacity=cap, \
	}

#define free_list(list, ...) { \
		free((list)->data); \
		(list)->length = 0; \
		(list)->capacity = 0; \
	}

#define list_isempty(list) \
	(!(list)->length)

#define list_length(list, type) \
	((list)->length / sizeof(type))

#define list_capacity(list, type) \
	((list)->capacity / sizeof(type))

#define list_first(list, type) \
	(*(type*)((list)->data))

#define list_last(list, type) \
	(*(type*)(&(list)->data[(list)->length - sizeof(type)]))

#define list_next(list, itr, type) \
	(((itr+1) >= ((type*)(&(list)->data[(list)->length])) ? (0) : (itr+1)))

#define list_end(list, type) \
	(*(type*)(&(list)->data[(list)->length]))

#define list_pop_front(list, type) { \
		(list)->data += sizeof(type); \
		(list)->length -= sizeof(type); \
	}

#define list_has(list, find, type) \
		(list_index_of((list), (find), type) != -1)

#define list_iterate(list, var_name, type) \
	type* var_name = (type*)((list)->data); \
	(ptrdiff_t)var_name != (ptrdiff_t)&(list)->data[(list)->length]; \
	var_name++

#define list_iterate_max(list, var_name, max, type) \
	type* var_name = (type*)((list)->data); \
	(((ptrdiff_t)var_name - (ptrdiff_t)(list)->data) / sizeof(type)) != max && (ptrdiff_t)var_name != (ptrdiff_t)&(list)->data[(list)->length]; \
	var_name++

#define list_iterate_condition(list, var_name, conditions, type) \
	type* var_name = (type*)((list)->data); \
	(conditions) && (ptrdiff_t)var_name != (ptrdiff_t)&(list)->data[(list)->length]; \
	var_name++

#define list_expand(list, size, type) { \
		if (list_capacity((list), type) < size) \
		{ \
			(list)->capacity = (sizeof(type) * size) + \
						(sizeof(type) * LIST_EXPAND_SIZE); \
			(list)->data = realloc(list->data, (list)->capacity); \
		} \
	}

#define list_push_back(list, value, type) { \
		if ((list)->length + sizeof(type) >= (list)->capacity) \
		{ \
			(list)->capacity += (sizeof(type) * LIST_EXPAND_SIZE); \
			(list)->data = realloc((list)->data, (list)->capacity); \
		} \
		(*(type*)(&(list)->data[(list)->length])) = *(type*)&(value); \
		(list)->length += sizeof(type); \
	}

#define list_clear(list, ...) { \
		(list)->length = 0; \
	}

#define list_set(list, value, idx, type) { \
		((type*)(list)->data)[(idx)] = *(type*)&(value); \
	}

#define list_get(list, idx, type) \
		(((type*)(list)->data)[(idx)])

#define list_index_of(list, find, type) \
	__list_index_of((list), (&(type){((type)find)}), sizeof(type))

#define list_replace(list, find, replace, type) { \
		for (intmax_t i = 0; i < list_length((list), type); i++) \
		{ \
			if (list_get((list), i, type) == (find)) \
			{ \
				list_set((list), (replace), i, type); \
				break; \
			} \
		} \
	}

#define list_pop_back(list, type) { \
		for (intmax_t i = 0; i < list_length((list), type); i++) \
			list_set((list), list_get((list), (i+1), type), i, type); \
		(list)->length -= sizeof(type); \
	}

#define list_remove(list, idx, type) { \
		for (intmax_t i = (idx); i < list_length((list), type); i++) \
			list_set((list), list_get((list), (i+1), type), i, type); \
		(list)->length -= sizeof(type); \
	}

#define list_add(dest, src, type) { \
		for (intmax_t i = 0; i < list_length((src), type); i++) \
			list_push_back((dest), list_get((src), i, type), type); \
	}

#define list_copy(dest, src, type) { \
		list_expand((dest), list_capacity((src), type), type); \
		for (intmax_t i = 0; i < list_length((src), type); i++) \
			list_push_back((dest), list_get((src), i, type), type) \
	}

#endif
