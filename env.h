#ifndef ENV_H
#define ENV_H

#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

typedef struct request_cost_t {
    int32_t connectMs;
    int32_t processMs;
    int32_t requestMs;
} request_cost_t;

typedef struct env_t {
    uint32_t concurrency;
    uint32_t totalRequestNum;
    uint32_t pendingRequestNum;
    uint32_t finishRequestNum;
    uint32_t thinkInterval;
    uint32_t incrementNum;

    char host[64];
    int port;
    char path[256];
    char query[256];

    struct {
        int32_t connectMeanMs;
        int32_t connectMaxMs;
        int32_t processMeanMs;
        int32_t processMaxMs;
        int32_t requestMeanMs;
        int32_t requestMaxMs;
        uint32_t requestCount;
    } statistic;

    request_cost_t maxConnect;
    request_cost_t maxProcess;
    request_cost_t maxRequest;
} env_t;

int env_parse_option(env_t* env, int argc, char* argv[]);

int64_t now_milliseconds();

#define errno_assert(x) \
    do {\
        if (!(x)) { \
            const char* errmsg = strerror(errno);\
            printf("%s:%d %s\\n", __FILE__, __LINE__, strerror(errno));\
        } \
        assert(x); \
    } while (0);

#endif
