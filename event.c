#include "event.h"
#include "env.h"

#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <signal.h>
#include <unistd.h>

#define EVENT_BUFF_SIZE 256

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

void event_poll()
{
    int err = 0;
    do  {
        err = event_poll_once();
    } while (!err);
}

int event_poll_once()
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


struct event_timer_t {
    int fd;
    event_timer_callback_t callback;
    void* userdata;
    event_watcher_t watcher;
};

static void on_timer_fd_readable(void* userdata, int evtSet)
{
    event_timer_t* timer = userdata;
    uint64_t value;
    int r = read(timer->fd, (char*)&value, sizeof(uint64_t));
    if (r > 0) {
        timer->callback(timer, timer->userdata);
    }
}

event_timer_t* event_set_timeout(event_timer_callback_t callback, void* userdata, int milliseconds)
{
    event_timer_t* timer = malloc(sizeof(event_timer_t));

    timer->fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);

    timer->callback = callback;
    timer->userdata = userdata;
    timer->watcher.callback = on_timer_fd_readable;
    timer->watcher.data = timer;

    event_add_watch(timer->fd, EVENT_READABLE, &timer->watcher);

    struct itimerspec v;
    v.it_value.tv_sec = milliseconds / 1000;
    v.it_value.tv_nsec = (milliseconds % 1000) * 1000 * 1000;
    v.it_interval.tv_sec = 0;
    v.it_interval.tv_nsec = 0;

    timerfd_settime(timer->fd, 0, &v, NULL);
    return timer;
}

event_timer_t* event_set_interval(event_timer_callback_t callback, void* userdata, int milliseconds)
{
    event_timer_t* timer = malloc(sizeof(event_timer_t));

    timer->fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);

    timer->callback = callback;
    timer->userdata = userdata;
    timer->watcher.callback = on_timer_fd_readable;
    timer->watcher.data = timer;

    event_add_watch(timer->fd, EVENT_READABLE, &timer->watcher);

    struct itimerspec v;
    v.it_value.tv_sec = milliseconds / 1000;
    v.it_value.tv_nsec = (milliseconds % 1000) * 1000 * 1000;
    v.it_interval.tv_sec = v.it_value.tv_sec;
    v.it_interval.tv_nsec = v.it_value.tv_nsec;

    timerfd_settime(timer->fd, 0, &v, NULL);
    return timer;
}

void event_release_timer(event_timer_t* timer)
{
    event_delete_watch(timer->fd);
    close(timer->fd);
    free(timer);
}


