#ifndef CANDLE_API_COMPILER_H
#define CANDLE_API_COMPILER_H

#include <stddef.h>

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#define struct_size(ptr, field, num) \
    (offsetof(__typeof(*(ptr)), field) + sizeof((ptr)->field[0]) * (num))

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define DECLARE_FLEX_ARRAY(type, name) \
    struct { \
        struct { } __empty_ ## name; \
        type name[]; \
    }

#define min(x, y)   ((x) < (y) ? (x) : (y))
#define max(x, y)   ((x) > (y) ? (x) : (y))

#endif // CANDLE_API_COMPILER_H
