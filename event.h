#ifndef EVENT_H
#define EVENT_H

void event_create();
void event_destroy();

void event_add(int fd);
void event_pool();

#endif
