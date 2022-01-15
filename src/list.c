#include "list.h"

intmax_t __list_index_of(struct list* list, void* find, intmax_t type_size)
{
	for (intmax_t i = 0; i < list->length; i += type_size)
	{
		if (0 == memcmp(&list->data[i], find, type_size))
		{
			return i / type_size;
		}
	}
	return -1;
}