#include "../inc/socket_reader.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>

#define MESSAGE_LEN_SIZE sizeof(uint32_t)

void socket_reader_init(socket_reader* socketReader, int socketFd)
{
    socketReader->socketFd = socketFd;

    socket_reader_reset(socketReader);
}

void socket_reader_reset(socket_reader* socketReader)
{
    socketReader->bytesExpected = MESSAGE_LEN_SIZE;
    socketReader->bytesReceived = 0;
}

int socket_reader_read(socket_reader* socketReader, buffer_span8* messageSpan)
{
    while (1)
    {
        while (socketReader->bytesReceived < socketReader->bytesExpected)
        {
            uint8_t* writePtr = socketReader->buffer + socketReader->bytesReceived;
            uint32_t bytesToRead = socketReader->bytesExpected - socketReader->bytesReceived;

            int result = recv(socketReader->socketFd, writePtr, bytesToRead, 0);

            if (result == 0)
            {
                // Socket closed
                return -1;
            }
            else if (result < 0)
            {
                if (errno != EWOULDBLOCK)
                {
                    // Error encountered
                    return -1;
                }

                return 0;
            }
            else
            {
                socketReader->bytesReceived += result;
            }
        }

        if (socketReader->bytesExpected == MESSAGE_LEN_SIZE)
        {
            // Finished reading message size
            const uint32_t* pMessageSizeNetwork = (uint32_t*)socketReader->buffer;
            const uint32_t messageSize = ntohl(*pMessageSizeNetwork);

            // Proceed to read message
            socketReader->bytesExpected = MESSAGE_LEN_SIZE + messageSize;
        }
        else
        {
            // Finished reading message
            const uint32_t messageSize = socketReader->bytesExpected - MESSAGE_LEN_SIZE;

            // Reset
            socket_reader_reset(socketReader);

            messageSpan->pointer = socketReader->buffer + MESSAGE_LEN_SIZE;
            messageSpan->length = messageSize;

            return 1;
        }
    }
}