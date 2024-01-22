#ifndef __MINDCONTROL_UTILS_H__
#define __MINDCONTROL_UTILS_H__

#include <stdarg.h>
#include <stdio.h>

#define IS_COMMAND(com, dat) (*(int*)(com) == *(int*)(dat))
#define UNSCALE_X(_x) ((_x) * g_screen_size.x / SCREEN_SCALE)
#define UNSCALE_Y(_y) ((_y) * g_screen_size.y / SCREEN_SCALE)
#define SCALE_X(_x) ((_x) * SCREEN_SCALE / g_screen_size.x)
#define SCALE_Y(_y) ((_y) * SCREEN_SCALE / g_screen_size.y)

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))

#ifndef ABS
	#define ABS(v) (((v) < 0) ? (-v) : (v))
#endif
#ifndef MAX
	#define MAX(a, b) (((a) < (b)) ? (b) : (a))
#endif
#ifndef MIN
	#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#define NOBJ(t) &(t){0}

#ifdef __WIN64

#include <windows.h>

#define SLEEP Sleep

#define THREAD DWORD

#define MAKE_THREAD() 0

#define THREAD_CALL(_thrd, _func, _arg) { \
		CreateThread(0, 0, (_func), (_arg), 0, (_thrd)); \
	}

#define THREAD_KILL(_thrd) { \
	ExitThread(*(_thrd)); \
}

#define CREATE_THREAD(_name, _arg_type, _arg_name, _code) \
	static DWORD WINAPI _name(LPVOID __varg) \
	{ \
		_arg_type _arg_name = *(_arg_type*)__varg; \
		_code \
	}

#endif

#ifdef __unix__

#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

#define SLEEP(_t) usleep((_t)*1000)
#define GET_MILLS() _get_mills()

static inline long long _get_mills(void) {
	struct timeval now;
	gettimeofday(&now, NULL);
	return now.tv_sec * 1000 + now.tv_usec / 1000;
}

#define THREAD pthread_t

#define MAKE_THREAD() 0

#define THREAD_CALL(_thrd, _func, _arg) { \
		pthread_create((_thrd), 0, (_func), (_arg)); \
	}

#define THREAD_KILL(_thrd) { \
	pthread_cancel(*(_thrd)); \
}

#define CREATE_THREAD(_name, _arg_type, _arg_name, _code) \
	static void* _name(void* __varg) \
	{ \
		_arg_type _arg_name = *(_arg_type*)__varg; \
		_code \
		return 0; \
	}

#endif

char* strrstr(char*, char*);
char* extract_command(char* _data);
void data_get_value(char** data, char* fmt, ...);
void data_get_string(char** data, char* output);
void load_shell_command(char* command, char* buffer, int length);
void load_formatted_shell_command(char* fmt, char* output_buffer, int output_buffer_length, ...);

#endif
