#include "connect.h"

#include <net_buffer.h>
#include <message_type.h>

#include "sales.h"

#include <arpa/inet.h>
#include <sys/socket.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct _client_socket_info
{
    int clientSocketFd;
    int clientSocketMonitorIndex;
} client_socket_info;

int connect_mgr_read_client_socket(connect_manager* connectManager, client_socket_info clientSocketInfo,
                                   socket_reader* reader);
int connect_mgr_parse_client_message(connect_manager* connectManager, client_socket_info clientSocketInfo,
                                     buffer_span8 messageSpan);
void accept_client_connection(connect_manager* connectManager, client_socket_info clientSocketInfo, int clientID);
void reject_client_connection(connect_manager* connectManager, client_socket_info clientSocketInfo);
void stop_monitor_client(connect_manager* connectManager, client_socket_info clientSocketInfo);

void connect_mgr_start_sales_mgr(int clientSocketFd);

void connect_mgr_init(connect_manager* connectManager)
{
    int serverSocketFd = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocketFd < 0)
    {
        perror("Cannot create socket");
        exit(1);
    }

    struct sockaddr_in serverAddr = {0};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVER_PORT);

    if (bind(serverSocketFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Socket binding failed");
        exit(1);
    }

    connectManager->listenSocketFd = serverSocketFd;
    memset(connectManager->clientsConnectionStatus, CONN_STATUS_DISCONNECTED,
           sizeof(connectManager->clientsConnectionStatus));

    const int pollFdListLength = sizeof(connectManager->pollFdList) / sizeof(connectManager->pollFdList[0]);
    struct pollfd* pollFdList = connectManager->pollFdList;

    // Monitor listen socket
    pollFdList[0].fd = connectManager->listenSocketFd;
    pollFdList[0].events = POLLIN;

    // Prepare client socket placeholders
    for (int i = 1; i < pollFdListLength + 1; i++)
    {
        pollFdList[i].fd = -1;
        pollFdList[i].events = POLLIN;
    }

    // Prepare socket buffers
    memset(connectManager->clientSocketReaders, 0, sizeof(connectManager->clientSocketReaders));
}

void connect_mgr_start(connect_manager* connectManager)
{
    const int listenSocketFd = connectManager->listenSocketFd;
    listen(listenSocketFd, MAX_CLIENT_COUNT);

    int oldSocketFdList[MAX_CLIENT_COUNT + 1];
    const int pollFdListLength = sizeof(connectManager->pollFdList) / sizeof(connectManager->pollFdList[0]);

    struct pollfd* pollFdList = connectManager->pollFdList;
    socket_reader** clientSocketReaders = connectManager->clientSocketReaders;

    while (1)
    {
        int pollRet = poll(pollFdList, pollFdListLength, -1);
        if (pollRet < 0)
        {
            perror("IO polling failed");
            exit(1);
        }

        if (pollRet == 0)
        {
            // Timed out
            continue;
        }

        // Copy fd list before iterating through
        for (int i = 0; i < pollFdListLength; i++)
        {
            oldSocketFdList[i] = pollFdList[i].fd;
        }

        for (int i = 0; i < pollFdListLength; i++)
        {
            struct pollfd* currentPollFd = &pollFdList[i];

            if (oldSocketFdList[i] < 0)
            {
                continue;
            }

            const short revents = currentPollFd->revents;
            if (revents & POLLNVAL)
            {
                perror("IO polling failed: POLLNVAL");
                exit(1);
            }

            if (revents & POLLERR)
            {
                perror("IO polling failed: POLLERR");
                exit(1);
            }

            if ((revents & POLLIN) == 0)
            {
                continue;
            }

            // Check if it was listen socket event
            if (i == 0)
            {
                struct sockaddr_in clientAddr = {0};
                socklen_t clientAddrSize = sizeof(clientAddr);

                int clientSocketFd = accept(listenSocketFd, (struct sockaddr*)&clientAddr, &clientAddrSize);

                if (clientSocketFd < 0)
                {
                    perror("Socket accept failed");
                    exit(1);
                }

                // Try adding newly connected fd to monitored list
                int found = 0;
                for (int i = 1; i < pollFdListLength; i++)
                {
                    // Find the first empty slot
                    if (pollFdList[i].fd < 0)
                    {
                        setsockopt(clientSocketFd, SOL_SOCKET, SOCK_NONBLOCK, NULL, 0);
                        pollFdList[i].fd = clientSocketFd;

                        // Allocate reader
                        clientSocketReaders[i] = (socket_reader*)malloc(sizeof(socket_reader));
                        socket_reader_init(clientSocketReaders[i], clientSocketFd);

                        found = 1;
                        break;
                    }
                }

                if (found == 0)
                {
                    close(clientSocketFd);
                }
            }
            else
            {
                client_socket_info clientSocketInfo = {.clientSocketFd = currentPollFd->fd,
                                                       .clientSocketMonitorIndex = i};
                // Handle client socket event
                int result = connect_mgr_read_client_socket(connectManager, clientSocketInfo, clientSocketReaders[i]);

                if (result < 0)
                {
                    stop_monitor_client(connectManager, clientSocketInfo);
                }
            }
        }
    }
}

int connect_mgr_read_client_socket(connect_manager* connectManager, client_socket_info clientSocketInfo,
                                   socket_reader* reader)
{
    while (1)
    {
        buffer_span8 messageSpan;
        int readResult = socket_reader_read(reader, &messageSpan);

        // Error
        if (readResult < 0)
        {
            return readResult;
        }

        // Partial read
        if (readResult == 0)
        {
            break;
        }

        // Full message read
        int resolved = connect_mgr_parse_client_message(connectManager, clientSocketInfo, messageSpan);
        if (resolved)
        {
            break;
        }
        
    }

    return 0;
}

int connect_mgr_parse_client_message(connect_manager* connectManager, client_socket_info clientSocketInfo,
                                     buffer_span8 messageSpan)
{
    net_buffer_context bufferContext;
    net_buffer_init(&bufferContext, messageSpan.pointer, NET_BUF_READ);

    message_type messageType = net_buffer_read8(&bufferContext);

    if (messageType == ConnectRequest)
    {
        uint8_t clientID = net_buffer_read8(&bufferContext);

        // Invalid client id
        if (clientID >= MAX_CLIENT_COUNT)
        {
            reject_client_connection(connectManager, clientSocketInfo);
        }
        else if (connectManager->clientsConnectionStatus[clientID] == CONN_STATUS_CONNECTED)
        {
            reject_client_connection(connectManager, clientSocketInfo);
        }
        else
        {
            accept_client_connection(connectManager, clientSocketInfo, clientID);
        }

        return 1;
    }
    else
    {
        printf("Unknown message type %d\n", messageType);
    }

    return 0;
}

void accept_client_connection(connect_manager* connectManager, client_socket_info clientSocketInfo, int clientID)
{
    uint8_t buf[32];
    net_buffer_context bufferContext;
    net_buffer_init(&bufferContext, buf, NET_BUF_WRITE);

    // Write message type
    net_buffer_write8(&bufferContext, ConnectResponse);

    // Write result
    net_buffer_write8(&bufferContext, 1);

    const int payloadSize = net_buffer_write_commit(&bufferContext);
    send(clientSocketInfo.clientSocketFd, buf, payloadSize, 0);

    // Update connect manager
    connectManager->clientsConnectionStatus[clientID] = 1;

    printf("Client #%d connected\n", clientID);
    connect_mgr_start_sales_mgr(clientSocketInfo.clientSocketFd);

    stop_monitor_client(connectManager, clientSocketInfo);
}

void reject_client_connection(connect_manager* connectManager, client_socket_info clientSocketInfo)
{
    uint8_t buf[32];
    net_buffer_context bufferContext;
    net_buffer_init(&bufferContext, buf, NET_BUF_WRITE);

    // Write message type
    net_buffer_write8(&bufferContext, ConnectResponse);

    // Write result
    net_buffer_write8(&bufferContext, 0);

    const int payloadSize = net_buffer_write_commit(&bufferContext);
    send(clientSocketInfo.clientSocketFd, buf, payloadSize, 0);

    stop_monitor_client(connectManager, clientSocketInfo);
}

void stop_monitor_client(connect_manager* connectManager, client_socket_info clientSocketInfo)
{
    const int fdIndex = clientSocketInfo.clientSocketMonitorIndex;

    connectManager->pollFdList[fdIndex].fd = -1;
    socket_reader** pClientSocketReader = &connectManager->clientSocketReaders[fdIndex];
    if (*pClientSocketReader != NULL)
    {
        free(*pClientSocketReader);
        *pClientSocketReader = NULL;
    }

    close(clientSocketInfo.clientSocketFd);
}

void connect_mgr_start_sales_mgr(int clientSocketFd)
{
    pid_t pid = fork();

    if (pid == -1)
    {
        perror("fork failed");
        exit(1);
    }

    if (pid == 0)
    {
        sales_manager salesManager;
        sales_mgr_init(&salesManager, clientSocketFd);
        sales_mgr_start(&salesManager);

        sales_mgr_free(&salesManager);

        exit(0);
    }
}