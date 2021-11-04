#ifndef __MINDCONTROL_UTILS_H__
#define __MINDCONTROL_UTILS_H__

#include <stdarg.h>
#include <stdio.h>

#define IS_COMMAND(com, dat) (*(int*)(com) == *(int*)(dat))
#define UNSCALE_X(x) ((x) * SCREEN_WIDTH / SCREEN_SCALE)
#define UNSCALE_Y(y) ((y) * SCREEN_HEIGHT / SCREEN_SCALE)
#define SCALE_X(x) ((x) * SCREEN_SCALE / SCREEN_WIDTH)
#define SCALE_Y(y) ((y) * SCREEN_SCALE / SCREEN_HEIGHT)

char* extract_command(char* _data);
void data_get_value(char** data, char* fmt, ...);

#endif