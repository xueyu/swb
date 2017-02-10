#ifndef ENV_H
#define ENV_H

#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

typedef struct env_t {
    uint32_t concurrency;
    uint32_t processCount;
    uint32_t durationSeconds;
    uint32_t totalRequestNum;
    uint32_t finishRequestNum;

    char host[64];
    int port;
    char path[256];
    char query[256];
} env_t;


#define errno_assert(x) \
    do {\
        if (!(x)) { \
            const char* errmsg = strerror(errno);\
            printf("%s:%d %s\\n", __FILE__, __LINE__, strerror(errno));\
        } \
        assert(x); \
    } while (0);

#endif
