#ifndef CLIENT_H
#define CLIENT_H

#include "env.h"

typedef struct client_t client_t;

client_t* client_create(env_t* env);

void client_destroy(client_t* client);


#endif
