#include "event.h"
#include <stdlib.h>
#include <sys/epoll.h>

typedef struct event_t {

    int epollFd;

} event_t;


event_t* event = NULL;

void event_create()
{
    event = malloc(sizeof(event_t));
}

void event_destroy()
{
    free(event);
}

void event_add(int fd)
{

}

void event_pool()
{

}
