#include "commodity_sales.h"

#include <message_type.h>
#include <net_buffer.h>
#include <socket_reader.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void commodity_sales_init(commodity_sales* commoditySales, int clientSocketFd, pipe_comm_channel communicationChannel)
{
    commoditySales->clientSocketFd = clientSocketFd;
    commoditySales->communicationChannel = communicationChannel;
}

void commodity_sales_start(commodity_sales* commoditySales)
{
    uint8_t interprocBuf[32];
    const int readFd = commoditySales->communicationChannel.readFd;
    const int writeFd = commoditySales->communicationChannel.writeFd;

    socket_reader* socketReader = (socket_reader*)malloc(sizeof(socket_reader));
    socket_reader_init(socketReader, commoditySales->clientSocketFd);

    while (1)
    {
        ssize_t readRet = read(readFd, interprocBuf, sizeof(interprocBuf));

        if (readRet < 0)
        {
            printf("[CommoditySales] Pipe read failed\n");
            exit(1);
        }

        if (readRet == 0)
        {
            printf("[CommoditySales] Pipe closed\n");
            exit(1);
        }

        // Foward message to socket
        send(commoditySales->clientSocketFd, interprocBuf, readRet, 0);

        while (1)
        {
            buffer_span8 messageSpan;

            int socketReadRet = socket_reader_read(socketReader, &messageSpan);
            if (socketReadRet < 0)
            {
                printf("[CommoditySales] Socket read error\n");
                exit(1);
            }
            else if (socketReadRet > 0)
            {
                // Full message read
                // Foward to pipe
                if (write(writeFd, messageSpan.pointer, messageSpan.length) < 0)
                {
                    printf("[CommoditySales] Pipe write failed\n");
                    exit(1);
                }

                break;
            }
        }
    }

    free(socketReader);
}