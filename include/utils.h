#ifndef __MINDCONTROL_UTILS_H__
#define __MINDCONTROL_UTILS_H__

#include <stdarg.h>
#include <stdio.h>

#define IS_COMMAND(com, dat) (*(int*)(com) == *(int*)(dat))
#define UNSCALE_X(_x) ((_x) * screen_size.x / SCREEN_SCALE)
#define UNSCALE_Y(_y) ((_y) * screen_size.y / SCREEN_SCALE)
#define SCALE_X(_x) ((_x) * SCREEN_SCALE / screen_size.x)
#define SCALE_Y(_y) ((_y) * SCREEN_SCALE / screen_size.y)

#ifdef __WIN64

#include <windows.h>

#define THREAD_CALL(_func, _arg) { \
		DWORD _thread; \
		CreateThread(0, 0, (_func), (_arg), 0, &_thread); \
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

#define THREAD_CALL(_func, _arg) { \
		pthread_t _thread; \
		pthread_create(&_thread, 0, (_func), (_arg)); \
	}

#define CREATE_THREAD(_name, _arg_type, _arg_name, _code) \
	void* _name(void* __varg) \
	{ \
		_arg_type _arg_name = *(_arg_type*)__varg; \
		_code \
	}

#endif

char* extract_command(char* _data);
void data_get_value(char** data, char* fmt, ...);

#endif