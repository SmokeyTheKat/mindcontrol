#ifndef __MINDCONTROL_UTILS_H__
#define __MINDCONTROL_UTILS_H__

#include <stdarg.h>
#include <stdio.h>

#define IS_COMMAND(com, dat) (*(int*)(com) == *(int*)(dat))
#define UNSCALE_X(_x) ((_x) * screen_size.x / SCREEN_SCALE)
#define UNSCALE_Y(_y) ((_y) * screen_size.y / SCREEN_SCALE)
#define SCALE_X(_x) ((_x) * SCREEN_SCALE / screen_size.x)
#define SCALE_Y(_y) ((_y) * SCREEN_SCALE / screen_size.y)

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
	DWORD WINAPI _name(LPVOID __varg) \
	{ \
		_arg_type _arg_name = *(_arg_type*)__varg; \
		_code \
	}

#endif

#ifdef __unix__

#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#define SLEEP(_t) usleep((_t)*1000)

#define THREAD pthread_t

#define MAKE_THREAD() 0

#define THREAD_CALL(_thrd, _func, _arg) { \
		pthread_create((_thrd), 0, (_func), (_arg)); \
	}

#define THREAD_KILL(_thrd) { \
	pthread_cancel(*(_thrd)); \
}

#define CREATE_THREAD(_name, _arg_type, _arg_name, _code) \
	void* _name(void* __varg) \
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

#endif