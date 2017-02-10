#include "client.h"
#include "event.h"
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>


struct client_t {
    int fd;
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

client_t* client_create(env_t* env)
{
    client_t* client = malloc(sizeof(client_t));

    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    errno_assert(clientfd > 0);
    struct sockaddr_in addr;

    convert_sockaddr(&addr, env->host, env->port);
    set_nonblocking(clientfd);
    
    event_add(clientfd);

    int rc = connect(clientfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
    if (rc == -1) {
        if (errno != EINPROGRESS) {
            printf("connect fail: %s\n", strerror(errno));
            free(client);
            return NULL;
        }
    }

    printf("connect success!\n");

    client->fd = clientfd;


    return client;
}

void client_destroy(client_t* client)
{
    shutdown(client->fd, SHUT_RDWR);
    free(client);
}
