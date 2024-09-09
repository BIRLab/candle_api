#ifndef CANDLE_API_COMPILER_H
#define CANDLE_API_COMPILER_H

#include <stddef.h>

#if defined(_MSC_VER)
#define typeof __typeof__
#define alloca _alloca
#endif

#if defined(__MINGW32__) || defined(__MINGW64__)
#error "MinGW is not support c11 threads.h"
#endif

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#define struct_size(ptr, field, num) \
    (offsetof(typeof(*(ptr)), field) + sizeof((ptr)->field[0]) * (num))

#define DECLARE_FLEX_ARRAY(type, name) \
    type name[0]

#ifndef min
#define min(x, y)   ((x) < (y) ? (x) : (y))
#endif

#ifndef max
#define max(x, y)   ((x) > (y) ? (x) : (y))
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#ifdef USING_TINYCTHREADS
#include "tinycthread.h"
#else
#include <threads.h>
#endif

#endif // CANDLE_API_COMPILER_H
