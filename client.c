#include "client.h"
#include "event.h"
#include "http_parser.h"

#include <stdlib.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFF_SIZE 2048
#define RECV_BUFF_SIZE 2048

typedef struct buff_item_t {
    struct buff_item_t* next;
    char data[BUFF_SIZE];
    uint32_t payload;
    uint32_t pos;
} buff_item_t;

typedef struct buff_t {
    buff_item_t* head;
    buff_item_t* tail;
} buff_t;

static buff_item_t* buff_create(buff_t* buff)
{
    buff_item_t* item = malloc(sizeof(buff_item_t));
    item->next = NULL;
    item->payload = 0u;
    item->pos = 0u;

    if (buff->head == NULL && buff->tail == NULL) {
        buff->head = item;
        buff->tail = item;
    } else {
        buff->tail->next = item;
    }
    return item;
}

static void buff_delete_front(buff_t* buff)
{
    if (buff->head != NULL) {
        buff_item_t* item = buff->head;
        if (buff->head == buff->tail) {
            buff->head = NULL;
            buff->tail = NULL;
        } else {
            buff->head = buff->head->next;
        }
        free(item);
    }
};

struct client_t {
    int fd;
    int pendingConnect;
    int connected;
    env_t* env;

    event_watcher_t watcher;

    http_parser_settings parserSettings;
    http_parser parser;

    buff_t sendBuff;
    char recvBuff[RECV_BUFF_SIZE];
};

static void convert_sockaddr(struct sockaddr_in* addr, const char* ipaddr, int port)
{
    memset(addr, 0, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    if (strcmp(ipaddr, "*") == 0 || strcmp(ipaddr, "any") == 0) {
        addr->sin_addr.s_addr = INADDR_ANY;
    } else {
        inet_aton(ipaddr, &(addr->sin_addr));
    }
    addr->sin_port = htons(port);
}

static void set_nonblocking(int fd)
{
    int opts = fcntl(fd, F_GETFL);
    errno_assert(opts != -1);

    opts = opts | O_NONBLOCK;
    int rc = 0;
    rc = fcntl(fd, F_SETFL, opts);
    errno_assert(rc != -1);
}

static void client_invoke_send(client_t* client)
{
    buff_item_t* item = client->sendBuff.head;
    while (item) {
        int c = send(client->fd, item->data + item->pos, item->payload - item->pos, 0);
        if (c > 0) {
            item->pos += c;
            if (item->pos == item->payload) {
                buff_item_t* next = item->next;
                buff_delete_front(&client->sendBuff);
                item = next;
            }
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                ;
            } else {
                perror("send error:");
            }
            break;
        }
    }
}

static void client_on_connect(client_t* client)
{
    client->connected = 1;
    buff_item_t* buff = buff_create(&client->sendBuff);
    int r = snprintf(buff->data, BUFF_SIZE, "GET %s?%s HTTP/1.1\r\nHOST: %s:%d\r\n\r\n", client->env->path, client->env->query, client->env->host, client->env->port);
    buff->payload = r;
    client_invoke_send(client);
}

static void client_on_connect_fail(client_t* client)
{
    printf("connect fail!\n");
}

static void client_on_close(client_t* client)
{
    if (client->connected) {
        client->connected = 0;
        printf("client on close\n");
    }
}

static void client_on_readable(client_t* client)
{
    while (1) {
        int r = recv(client->fd, client->recvBuff, RECV_BUFF_SIZE, 0);
        if (r == 0) {
            client_on_close(client);
            break;
        } else if (r <= -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                ;
            } else {
                if (client->pendingConnect) {
                    client->pendingConnect = 0;
                    client_on_connect_fail(client);
                } else {
                    client_on_close(client);
                }
            }
            break;
        } else {
            int n = http_parser_execute(&client->parser, &client->parserSettings, client->recvBuff, r);
            if (n != r) {
                // TODO
            }
        }
    }
}

static void client_on_writable(client_t* client)
{
    if (client->pendingConnect) {
        client->pendingConnect = 0;
        client_on_connect(client);
    } else {
        client_invoke_send(client);
    }
}

static void client_on_event_callback(void* userdata, int evtSet)
{
    client_t* client = userdata;

    if (evtSet & EVENT_READABLE) {
        client_on_readable(client);
    } else if (evtSet & EVENT_WRITABLE) {
        client_on_writable(client);
    }
}

static int http_on_message_begin_cb(http_parser* parser)
{
    return 0;
}

static int http_on_status(http_parser* parser, const char* at, size_t len)
{
    return 0;
}

static int http_on_header_field(http_parser* parser, const char* at, size_t len)
{
    return 0;
}

static int http_on_header_value(http_parser* parser, const char* at, size_t len)
{
    return 0;
}

static int http_on_headers_complete(http_parser* parser)
{
    return 0;
}

static int http_on_body_cb(http_parser* parser, const char* at, size_t len)
{
    return 0;
}

static int http_on_message_complete_cb(http_parser* parser)
{
    printf("finish!\n");
    return 0;
}

client_t* client_create(env_t* env)
{
    client_t* client = malloc(sizeof(client_t));
    client->connected = 0;
    client->pendingConnect = 1;
    client->watcher.callback = client_on_event_callback;
    client->watcher.data = client;
    client->sendBuff.head = NULL;
    client->sendBuff.tail = NULL;
    client->env = env;

    memset(&client->parserSettings, 0, sizeof(client->parserSettings));
    client->parserSettings.on_status = http_on_status;
    client->parserSettings.on_header_field = http_on_header_field;
    client->parserSettings.on_header_value = http_on_header_value;
    client->parserSettings.on_headers_complete = http_on_headers_complete;
    client->parserSettings.on_body = http_on_body_cb;
    client->parserSettings.on_message_begin = http_on_message_begin_cb;
    client->parserSettings.on_message_complete = http_on_message_complete_cb;

    http_parser_init(&client->parser, HTTP_RESPONSE);
    client->parser.data = client;

    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    client->fd = clientfd;
    errno_assert(clientfd > 0);
    struct sockaddr_in addr;

    convert_sockaddr(&addr, env->host, env->port);
    set_nonblocking(clientfd);

    printf("connect %s:%d\n", env->host, env->port);

    event_add_watch(clientfd, EVENT_READABLE | EVENT_WRITABLE, &client->watcher);

    int rc = connect(clientfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
    if (rc == -1) {
        if (errno != EINPROGRESS) {
            printf("connect fail: %s\n", strerror(errno));
            free(client);
            return NULL;
        }
    } else {
        client_on_connect(client);
    }

    return client;
}

void client_destroy(client_t* client)
{
    shutdown(client->fd, SHUT_RDWR);
    free(client);

    printf("client_destroy\n");
}
