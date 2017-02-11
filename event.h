#ifndef EVENT_H
#define EVENT_H

enum event_type {
    EVENT_READABLE  = 1,
    EVENT_WRITABLE  = 2
};

typedef void (*event_callback_t)(void* userdata, int evtSet);

typedef struct event_watcher_t {
    void* data;
    event_callback_t callback;
} event_watcher_t;

void event_create();
void event_destroy();

void event_add_watch(int fd, int evtFlag, event_watcher_t* watcher);
void event_update_watch(int fd, int evtFlag, event_watcher_t* watcher);
void event_delete_watch(int fd);

void event_pool();
int event_pool_once();

#endif
