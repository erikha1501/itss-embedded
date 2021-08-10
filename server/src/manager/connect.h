#pragma once

#include "../constants.h"
#include <socket_reader.h>

#include <poll.h>

#include <stdint.h>

typedef enum _connection_status
{
    CONN_STATUS_DISCONNECTED,
    CONN_STATUS_CONNECTED
} connection_status;

typedef struct _connect_manager
{
    int listenSocketFd;

    connection_status clientsConnectionStatus[MAX_CLIENT_COUNT];

    struct pollfd pollFdList[MAX_CLIENT_COUNT + 1];

    socket_reader* clientSocketReaders[MAX_CLIENT_COUNT + 1];
} connect_manager;

void connect_mgr_init(connect_manager* connectManager);
void connect_mgr_start(connect_manager* connectManager);