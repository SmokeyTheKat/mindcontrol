#ifndef __MINDCONTROL_MCERROR_H__
#define __MINDCONTROL_MCERROR_H__

#include <stdarg.h>

#ifdef MC_DEBUG
#define mcerror(format, ...) { printf(format,  __VA_ARGS__); exit(1); }
#else
#define mcerror(format, ...) { printf(format,  __VA_ARGS__); }
#endif

#endif
