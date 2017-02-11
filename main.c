#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>

#include "http_parser.h"
#include "env.h"
#include "client.h"
#include "event.h"

static int parse_url(env_t* env, const char* url)
{
    struct http_parser_url parser;
    http_parser_url_init(&parser);

    if (http_parser_parse_url(url, strlen(url), 0, &parser)) {
        return 1;
    }

    if (parser.field_set & 1 << UF_HOST) {
        strncpy(env->host, url + parser.field_data[UF_HOST].off, parser.field_data[UF_HOST].len);
    } else {
        return 1;
    }

    if (parser.field_set & 1 << UF_PATH) {
        strncpy(env->path, url + parser.field_data[UF_PATH].off, parser.field_data[UF_PATH].len);
    } else {
        strcpy(env->path, "/");
    }

    if (parser.field_set & 1 << UF_QUERY) {
        strncpy(env->query, url + parser.field_data[UF_QUERY].off, parser.field_data[UF_QUERY].len);
    }

    env->port = parser.port;
    if (env->port == 0) {
        env->port = 80;
    }
    return 0;
}

static int parse_option(env_t* env, int argc, char* argv[])
{
    int ch;
    while ((ch = getopt(argc, argv, "c:n:t:p:")) != -1) {
        switch (ch) {
        case 'c':
            env->concurrency = strtoul(optarg, NULL, 10);
            break;
        case 'n':
            env->totalRequestNum = strtoul(optarg, NULL, 10);
            break;
        case 'p':
            env->processCount = strtoul(optarg, NULL, 10);
            break;
        case 't':
            env->durationSeconds = strtoul(optarg, NULL, 10);
            break;
        default:
            break;
        }
    }

    if (optind == 0) {
        perror("bad url value\n");
        return 1;
    }

    char* url = argv[optind];
    if (parse_url(env, url) != 0) {
        printf("parse url fail:%s\n", url);
        return 1;
    }

    if (env->concurrency == 0) {
        perror("bad concurrency value\n");
        return 1;
    }
    if (env->durationSeconds == 0 && env->totalRequestNum == 0) {
        perror("bad durationSeconds or totalRequestNum value\n");
        return 1;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    struct env_t env;
    memset(&env, 0, sizeof(struct env_t));

    // -c currency
    // -n total request num
    // -t duration seconds
    // -p process
    if (parse_option(&env, argc, argv) != 0) {
        return 1;
    }

    event_create();

    client_t** clientArr = malloc(env.concurrency* sizeof(client_t*));
    for (int i = 0; i < env.concurrency; ++i) {
        client_t* client = client_create(&env);
        clientArr[i] = client;
    }

    int err = 0;
    do {
        err = event_pool_once();
    } while (!err);

    for (int i = 0; i < env.concurrency; ++i) {
        if (clientArr[i]) {
            client_destroy(clientArr[i]);
        }
    }
    free(clientArr);

    event_destroy();

    return 0;
}
