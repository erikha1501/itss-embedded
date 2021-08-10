#pragma once

#include <stdint.h>

#define SOCKET_BUFFER_CAPACITY 256

typedef struct _buffer_span8
{
    uint8_t* pointer;
    uint32_t length;
} buffer_span8;

typedef struct _socket_reader
{
    int socketFd;

    uint32_t bytesReceived;
    uint32_t bytesExpected;

    uint8_t buffer[SOCKET_BUFFER_CAPACITY];
} socket_reader;

void socket_reader_init(socket_reader* socketReader, int socketFd);
void socket_reader_reset(socket_reader* socketReader);

// return values:
// -1       -> error
//  0       -> partial read
//  1       -> full message read
int socket_reader_read(socket_reader* socketReader, buffer_span8* messageSpan);