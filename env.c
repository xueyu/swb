#include "env.h"
#include "http_parser.h"
#include <getopt.h>
#include <stdlib.h>
#include <time.h>

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

static void echo_help()
{
    printf("swb help:\n");
    printf("-c concurrency num\n");
    printf("-n total requst num\n");
    printf("-i think interval (milliseconds)\n");
    printf("-m increment num in every think\n");
}

int env_parse_option(env_t* env, int argc, char* argv[])
{
    env->incrementNum = 100;
    env->thinkInterval = 100;
    int ch;
    while ((ch = getopt(argc, argv, "c:n:i:m:h")) != -1) {
        switch (ch) {
        case 'c':
            env->concurrency = strtoul(optarg, NULL, 10);
            break;
        case 'n':
            env->totalRequestNum = strtoul(optarg, NULL, 10);
            break;
        case 'i':
            env->thinkInterval = strtoul(optarg, NULL, 10);
            break;
        case 'm':
            env->incrementNum = strtoul(optarg, NULL, 10);
            break;
        case 'h':
        case '?':
            echo_help();
            return 1;
        default:
            break;
        }
    }

    if (optind <= 1 || optind >= argc) {
        printf("bad url value\n");
        return 1;
    }

    char* url = argv[optind];
    if (parse_url(env, url) != 0) {
        printf("parse url fail:%s\n", url);
        return 1;
    }

    if (env->concurrency == 0) {
        printf("bad concurrency value\n");
        return 1;
    }
    if (env->totalRequestNum == 0) {
        printf("bad totalRequestNum value\n");
        return 1;
    }
    return 0;
}

int64_t now_milliseconds()
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return now.tv_sec * 1000 + now.tv_nsec / 1000000;
}
