#include "sales.h"

#include <commodity_info.h>
#include <message_type.h>
#include <net_buffer.h>

#include <assert.h>
#include <poll.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

int sales_mgr_read_client_socket(sales_manager* salesManager, socket_reader* reader);
void sales_mgr_parse_client_message(sales_manager* salesManager, buffer_span8 messageSpan);
void handle_sales_request(sales_manager* salesManager, uint8_t clientID, const uint8_t* salesInformation);
void handle_inventory_fetch(sales_manager* salesManager, uint8_t clientID);


void sales_mgr_init(sales_manager* salesManager, int clientSocketFd)
{
    salesManager->clientSocketFd = clientSocketFd;
    salesManager->socketReader = NULL;

    equip_info_init(&salesManager->equipInfo, EQUIP_INFO_GET);
}

void sales_mgr_start(sales_manager* salesManager)
{
    // Allocate reader
    salesManager->socketReader = (socket_reader*)malloc(sizeof(socket_reader));
    socket_reader_init(salesManager->socketReader, salesManager->clientSocketFd);

    struct pollfd pollFd = {0};
    pollFd.fd = salesManager->clientSocketFd;
    pollFd.events = POLLIN;

    while (1)
    {
        int pollRet = poll(&pollFd, 1, -1);
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

        const short revents = pollFd.revents;
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

        int result = sales_mgr_read_client_socket(salesManager, salesManager->socketReader);

        if (result < 0)
        {
            printf("Client socket closed\n");
            break;
        }
    }

    free(salesManager->socketReader);
    salesManager->socketReader = NULL;
}

int sales_mgr_read_client_socket(sales_manager* salesManager, socket_reader* reader)
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
        sales_mgr_parse_client_message(salesManager, messageSpan);
    }

    return 0;
}

void sales_mgr_parse_client_message(sales_manager* salesManager, buffer_span8 messageSpan)
{
    net_buffer_context bufferContext;
    net_buffer_init(&bufferContext, messageSpan.pointer, NET_BUF_READ);

    // Read message type
    message_type messageType = net_buffer_read8(&bufferContext);

    if (messageType == SalesRequest)
    {
        assert(messageSpan.length == 5);

        // Read client id
        uint8_t clientID = net_buffer_read8(&bufferContext);

        // Invalid client id
        if (clientID >= MAX_CLIENT_COUNT)
        {
            printf("Invalid client id\n");
            return;
        }

        printf("[SalesManager] Client #%d issues sales request\n", clientID);
        handle_sales_request(salesManager, clientID, net_buffer_get_current(&bufferContext));
    }
    else if (messageType == InventoryFetchRequest)
    {
        assert(messageSpan.length == 2);

        // Read client id
        uint8_t clientID = net_buffer_read8(&bufferContext);

        // Invalid client id
        if (clientID >= MAX_CLIENT_COUNT)
        {
            printf("Invalid client id\n");
            return;
        }

        handle_inventory_fetch(salesManager,  clientID);
    }
    else
    {
        printf("Unknown message type %d\n", messageType);
    }
}

void handle_sales_request(sales_manager* salesManager, uint8_t clientID, const uint8_t* salesInformation)
{
    uint8_t buf[32];
    net_buffer_context bufferContext;
    net_buffer_init(&bufferContext, buf, NET_BUF_WRITE);

    equip_info* equipInfo = &salesManager->equipInfo;
    int result = equip_info_apply_sales(equipInfo, clientID, salesInformation);

    // Write message type
    net_buffer_write8(&bufferContext, SalesResponse);

    // Write response code
    if (result < 0)
    {
        net_buffer_write8(&bufferContext, 0);
    }
    else if (result == 0)
    {
        net_buffer_write8(&bufferContext, 1);
    }
    else
    {
        net_buffer_write8(&bufferContext, 2);
    }

    uint8_t inventoryInfo[COMMODITY_COUNT];
    equip_info_read(equipInfo, clientID, inventoryInfo);

    for (int i = 0; i < COMMODITY_COUNT; i++)
    {
        net_buffer_write8(&bufferContext, inventoryInfo[i]);
    }

    int payloadSize = net_buffer_write_commit(&bufferContext);
    send(salesManager->clientSocketFd, buf, payloadSize, 0);
}

void handle_inventory_fetch(sales_manager* salesManager, uint8_t clientID)
{
    uint8_t buf[32];
    net_buffer_context bufferContext;
    net_buffer_init(&bufferContext, buf, NET_BUF_WRITE);

    equip_info* equipInfo = &salesManager->equipInfo;

    uint8_t inventoryInfo[COMMODITY_COUNT];
    equip_info_read(equipInfo, clientID, inventoryInfo);

    // Write message type
    net_buffer_write8(&bufferContext, InventoryFetchResponse);

    // Write stock info
    for (int i = 0; i < COMMODITY_COUNT; i++)
    {
        net_buffer_write8(&bufferContext, inventoryInfo[i]);
    }

    int payloadSize = net_buffer_write_commit(&bufferContext);
    send(salesManager->clientSocketFd, buf, payloadSize, 0);
}

void sales_mgr_free(sales_manager* salesManager)
{
    close(salesManager->clientSocketFd);
    salesManager->clientSocketFd = -1;

    equip_info_free(&salesManager->equipInfo, 0);
}