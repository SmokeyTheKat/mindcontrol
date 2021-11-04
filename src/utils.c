#include "utils.h"

char* extract_command(char* _data)
{
	static char* data = 0;
	if (_data != 0) data = _data;
	if (data == 0) return 0;

	while (*data)
	{
		if (*data == '[') return (data++) + 1;
		data++;
	}
	return 0;
}

void data_get_value(char** data, char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	(*data) += 4;
	vsscanf(*data, fmt, args);

	va_end(args);
}
