#include "event.h"
#include <stdlib.h>
#include <sys/epoll.h>
#include <signal.h>
#include <unistd.h>
#include "env.h"

#define EVENT_BUFF_SIZE 64

struct event_t {

    int epollFd;

    int exit;

    struct epoll_event evtBuff[EVENT_BUFF_SIZE];
};

static struct event_t* event = NULL;

static void signal_handler(int sig)
{
    if (sig == SIGINT) {
        event->exit = 1;
    }
}

void event_create()
{
    event = malloc(sizeof(struct event_t));

    event->epollFd = epoll_create(9999);
    event->exit = 0;
    errno_assert(event->epollFd > 0);

    signal(SIGINT, signal_handler);
    signal(SIGPIPE, SIG_IGN);
}

void event_destroy()
{
    close(event->epollFd);
    free(event);
}

void event_add_watch(int fd, int evtFlag, event_watcher_t* watcher)
{
    struct epoll_event ee;
    ee.events = EPOLLET
                | (evtFlag & EVENT_READABLE?  EPOLLIN : 0)
                | (evtFlag & EVENT_WRITABLE ? EPOLLOUT : 0);
    ee.data.ptr = watcher;
    int r = epoll_ctl(event->epollFd, EPOLL_CTL_ADD, fd, &ee);
    errno_assert(r == 0);
}

void event_update_watch(int fd, int evtFlag, event_watcher_t* watcher)
{
    struct epoll_event ee;
    ee.events = EPOLLET
                | (evtFlag & EVENT_READABLE?  EPOLLIN : 0)
                | (evtFlag & EVENT_WRITABLE ? EPOLLOUT : 0);
    ee.data.ptr = watcher;
    int r = epoll_ctl(event->epollFd, EPOLL_CTL_MOD, fd, &ee);
    errno_assert(r == 0);
}

void event_delete_watch(int fd)
{
    struct epoll_event ee;
    int r = epoll_ctl(event->epollFd, EPOLL_CTL_DEL, fd, &ee);
    errno_assert(r == 0);
}

void event_pool()
{
    int err = 0;
    do  {
        err = event_pool_once();
    } while (!err);
}

int event_pool_once()
{
    int n = epoll_wait(event->epollFd, event->evtBuff, EVENT_BUFF_SIZE, 25);

    if (n <= -1) {
        return 1;
    } else if (n == 0) {
        // time out
    } else {
        for (int i = 0; i < n; ++i) {
            int evtSet = 0;
            uint32_t events = event->evtBuff[i].events;
            if ((events & EPOLLIN) || (events & EPOLLERR)) {
                evtSet |= EVENT_READABLE;
            }
            if (events & EPOLLOUT) {
                evtSet |= EVENT_WRITABLE;
            }
            event_watcher_t* watcher = event->evtBuff[i].data.ptr;
            watcher->callback(watcher->data, evtSet);
        }
    }
    return 0;
}
