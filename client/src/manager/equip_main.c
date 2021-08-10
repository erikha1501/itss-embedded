#include "equip_main.h"

#include "commodity_sales.h"

#include <message_type.h>
#include <net_buffer.h>
#include <socket_reader.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SERVER_PORT 6969

void send_connection_request(const equip_main* equipMain, int clientSocketFd);
int wait_for_connection_response(int clientSocketFd);
int try_read_response_message(socket_reader* socketReader);

void start_commodity_sales(int clientSocketFd, pipe_comm_channel* communicationChannel);

void equip_main_init(equip_main* equipMain, uint8_t clientID)
{
    equipMain->clientID = clientID;
}

int equip_main_establish_connection(equip_main* equipMain)
{
    int clientSocketFd = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocketFd < 0)
    {
        perror("Cannot create socket");
        exit(1);
    }

    struct sockaddr_in serverAddr = {0};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVER_PORT);

    if (connect(clientSocketFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Socket connect failed");
        exit(1);
    }

    send_connection_request(equipMain, clientSocketFd);
    int waitResult = wait_for_connection_response(clientSocketFd);

    if (waitResult == 1)
    {
        start_commodity_sales(clientSocketFd, &equipMain->communicationChannel);
    }

    close(clientSocketFd);

    return waitResult;
}

void send_connection_request(const equip_main* equipMain, int clientSocketFd)
{
    uint8_t buf[32];
    net_buffer_context bufferWriteContext;
    net_buffer_init(&bufferWriteContext, buf, NET_BUF_WRITE);

    net_buffer_write8(&bufferWriteContext, ConnectRequest);
    net_buffer_write8(&bufferWriteContext, equipMain->clientID);

    const int payloadSize = net_buffer_write_commit(&bufferWriteContext);
    send(clientSocketFd, buf, payloadSize, 0);
}

int wait_for_connection_response(int clientSocketFd)
{
    // Allocate socket reader
    socket_reader* socketReader = (socket_reader*)malloc(sizeof(socket_reader));
    socket_reader_init(socketReader, clientSocketFd);

    int waitResult = 0;

    while (1)
    {
        int readResult = try_read_response_message(socketReader);

        // Reponse message received or error occured
        if (readResult != 0)
        {
            waitResult = readResult;
            break;
        }
    }

    free(socketReader);

    return waitResult;
}

int try_read_response_message(socket_reader* socketReader)
{
    buffer_span8 messageSpan;
    int readResult = socket_reader_read(socketReader, &messageSpan);

    // Error
    if (readResult < 0)
    {
        return readResult;
    }

    // Partial read
    if (readResult == 0)
    {
        return 0;
    }

    // Full message read
    net_buffer_context bufferReadContext;
    net_buffer_init(&bufferReadContext, messageSpan.pointer, NET_BUF_READ);

    message_type messageType = net_buffer_read8(&bufferReadContext);
    if (messageType == ConnectResponse)
    {
        uint8_t connectResult = net_buffer_read8(&bufferReadContext);
        if (connectResult == 1)
        {
            return 1;
        }
    }
    else
    {
        printf("Unknown message type %d\n", messageType);
    }

    return 0;
}

void start_commodity_sales(int clientSocketFd, pipe_comm_channel* communicationChannel)
{
    // Prepare communication channel
    int p2cFds[2];
    int c2pFds[2];

    if (pipe(p2cFds) < 0)
    {
        printf("Cannot create pipe\n");
        exit(1);
    }

    if (pipe(c2pFds) < 0)
    {
        printf("Cannot create pipe\n");
        exit(1);
    }

    // Fork
    pid_t pid = fork();

    if (pid == -1)
    {
        perror("fork failed");
        exit(1);
    }

    if (pid == 0)
    {
        close(p2cFds[1]);
        close(c2pFds[0]);

        pipe_comm_channel communicationChannel = {.readFd = p2cFds[0], .writeFd = c2pFds[1]};

        commodity_sales commoditySales;
        commodity_sales_init(&commoditySales, clientSocketFd, communicationChannel);
        commodity_sales_start(&commoditySales);

        exit(0);
    }
    else
    {
        close(p2cFds[0]);
        close(c2pFds[1]);

        *communicationChannel = (pipe_comm_channel){.readFd = c2pFds[0], .writeFd = p2cFds[1] };
    }
}
