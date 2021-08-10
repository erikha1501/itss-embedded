#pragma once

typedef struct _pipe_comm_channel
{
    int readFd;
    int writeFd;
} pipe_comm_channel;