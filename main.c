#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "http_parser.h"
#include "env.h"
#include "client.h"
#include "event.h"

static struct env_t env;
static client_t** clientArr;
static event_timer_t* timer;
static int stop = 0;

static void dump_statistic()
{
    printf("finish request: %u\n", env.statistic.requestCount);
    printf("connect time: mean=%dms, max=%dms\n", env.statistic.connectMeanMs, env.statistic.connectMaxMs);
    printf("process time: mean=%dms, max=%dms\n", env.statistic.processMeanMs, env.statistic.processMaxMs);
    printf("request time: mean=%dms, max=%dms\n", env.statistic.requestMeanMs, env.statistic.requestMaxMs);

    printf("the slowest connect one: connect=%d, process=%d, request=%d\n", env.maxConnect.connectMs, env.maxConnect.processMs, env.maxConnect.requestMs);
    printf("the slowest process one: connect=%d, process=%d, request=%d\n", env.maxProcess.connectMs, env.maxProcess.processMs, env.maxProcess.requestMs);
    printf("the slowest request one: connect=%d, process=%d, request=%d\n", env.maxRequest.connectMs, env.maxRequest.processMs, env.maxRequest.requestMs);
}

static void on_time_interval(event_timer_t* timer0, void* userdata)
{
    // 增量产生压力
    int incrementCount = 0;
    for (int i = 0; i < env.concurrency; ++i) {
        if (clientArr[i] == NULL) {
            if (incrementCount < env.incrementNum) {
                client_t* client = client_create(&env);
                if (client) {
                    ++incrementCount;
                    clientArr[i] = client;
                }
            }
        } else {
            if (env.pendingRequestNum + env.finishRequestNum < env.totalRequestNum) {
                client_update(clientArr[i]);
            }
            if (env.finishRequestNum >= env.totalRequestNum) {
                stop = 1;
            }
        }
    }

    if (stop) {
        event_release_timer(timer);
        timer = NULL;
        dump_statistic();
    }
}

int main(int argc, char* argv[])
{
    memset(&env, 0, sizeof(struct env_t));

    // -c currency
    // -n total request num
    // -t duration seconds
    // -p process
    if (env_parse_option(&env, argc, argv) != 0) {
        return 1;
    }

    printf("concurrency=%u, thinkInterval=%u, incrementNum=%u, totalRequestNum=%u\n", env.concurrency, env.thinkInterval, env.incrementNum, env.totalRequestNum);

    event_create();

    clientArr = malloc(env.concurrency* sizeof(client_t*));
    for (int i = 0; i < env.concurrency; ++i) {
        clientArr[i] = NULL;
    }

    timer = event_set_interval(on_time_interval, NULL, env.thinkInterval);

    int err = 0;
    do {
        err = event_poll_once();
    } while (!err && stop == 0);

    int existClients = 0;
    for (int i = 0; i < env.concurrency; ++i) {
        if (clientArr[i]) {
            ++existClients;
            client_destroy(clientArr[i]);
        }
    }
    free(clientArr);
    if (timer) {
        event_release_timer(timer);
    }

    printf("pendingRequestNum=%u, finishRequestNum=%u, existClients=%d\n", env.pendingRequestNum, env.finishRequestNum, existClients);

    event_destroy();

    return 0;
}
