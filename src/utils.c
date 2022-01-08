#include "utils.h"

#include <string.h>
#include <stddef.h>


char* strrstr(char* str, char* chrs)
{
	int str_len = strlen(str);
	int chrs_len = strlen(chrs);
	for (int i = str_len - 1; i >= 0; i--)
		for (int j = 0; j < chrs_len; j++)
			if (str[i] == chrs[j])
				return &str[i];
	return 0;
}

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

void data_get_string(char** data, char* output)
{
	(*data) += 4;

	char* str_start = *data;
	char* str_end = *data;
	while (*(str_end) != 1) str_end++;
	ptrdiff_t str_length = ((ptrdiff_t)str_end) - ((ptrdiff_t)str_start);
	strncpy(output, str_start, str_length);
	output[str_length] = 0;
	str_end++;
	(*data) = str_end;
}

void load_shell_command(char* command, char* buffer, int length)
{
	FILE* fp = popen(command, "r");
	int read_len = 0;
	int out_length = 0;
	while ((read_len = fread(&buffer[out_length], 1, length, fp)) > 0)
		out_length += read_len;
	if (buffer[out_length-1] == '\n')
		buffer[out_length-1] = 0;
	pclose(fp);
}
