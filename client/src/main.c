#include "manager/equip_main.h"

#include <commodity_info.h>
#include <message_type.h>
#include <net_buffer.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SERVER_PORT 6969

const char* commodityNames[COMMODITY_COUNT] = {COMMODITY_NAME_1, COMMODITY_NAME_2, COMMODITY_NAME_3};

int prompt_menu(const uint8_t* stockInfo, uint8_t* salesInfo);
void print_menu(const uint8_t* stockInfo);

int send_inventory_fetch_request(equip_main* equipMain, uint8_t* stockInfo);
int send_sales_request(equip_main* equipMain, const uint8_t* salesInfo, uint8_t* newStockInfo);

int main()
{
    printf("Enter client ID: ");
    int clientID;
    scanf(" %d", &clientID);

    equip_main equipMain;
    equip_main_init(&equipMain, clientID);

    if (equip_main_establish_connection(&equipMain) != 1)
    {
        printf("Cannot establish connection to server\n");
        exit(1);
    }

    uint8_t stockInfo[COMMODITY_COUNT] = {0};
    uint8_t salesInfo[COMMODITY_COUNT] = {0};

    int fetchResult = send_inventory_fetch_request(&equipMain, stockInfo);
    if (fetchResult != 1)
    {
        printf("Cannot fetch inventory info\n");
        exit(1);
    }

    while (1)
    {
        int promptResult = prompt_menu(stockInfo, salesInfo);
        if (promptResult < 0)
        {
            break;
        }
        else if (promptResult > 0)
        {
            int salesResult = send_sales_request(&equipMain, salesInfo, stockInfo);

            if (salesResult == 0)
            {
                printf("Invalid purchase\n");
            }
            else if (salesResult == 1)
            {
                printf("Ok\n");
            }
            else if (salesResult == 2)
            {
                printf("Replenishing...\n");
            }
            else
            {
                printf("Error: Server responses with unknown value of %d\n", (int)salesResult);
                exit(1);
            }

            // Reset sales info
            for (int i = 0; i < COMMODITY_COUNT; i++)
            {
                salesInfo[i] = 0;
            }
        }
    }

    return 0;
}

int prompt_menu(const uint8_t* stockInfo, uint8_t* salesInfo)
{
    print_menu(stockInfo);

    int choice;
    scanf(" %d", &choice);

    if (choice == 0)
    {
        return -1;
    }

    if (choice >= 1 && choice <= COMMODITY_COUNT)
    {
        salesInfo[choice - 1] = 1;
        return 1;
    }
    else
    {
        printf("Invalid choice\n");
    }

    return 0;
}

void print_menu(const uint8_t* stockInfo)
{
    printf("\n====MENU====\n");

    for (int i = 0; i < COMMODITY_COUNT; i++)
    {
        printf("%d. %s (%d)\n", i + 1, commodityNames[i], (int)stockInfo[i]);
    }

    printf("\n");
    printf("0. Exit\n");
    printf("\n");

    printf("Enter choice: ");
}

int send_inventory_fetch_request(equip_main* equipMain, uint8_t* stockInfo)
{
    const int readFd = equipMain->communicationChannel.readFd;
    const int writeFd = equipMain->communicationChannel.writeFd;

    uint8_t buf[32];

    net_buffer_context bufferWriteContext;
    net_buffer_init(&bufferWriteContext, buf, NET_BUF_WRITE);

    // Wrtie message type
    net_buffer_write8(&bufferWriteContext, InventoryFetchRequest);

    // Write client id
    net_buffer_write8(&bufferWriteContext, equipMain->clientID);

    int payloadSize = net_buffer_write_commit(&bufferWriteContext);
    if (write(writeFd, buf, payloadSize) < 0)
    {
        printf("[EquipMain] Cannot write to pipe\n");
        exit(1);
    }

    int readRet = read(readFd, buf, sizeof(buf));

    if (readRet < 0)
    {
        printf("[EquipMain] Cannot read from pipe\n");
        exit(1);
    }

    net_buffer_context bufferReadContext;
    net_buffer_init(&bufferReadContext, buf, NET_BUF_READ);

    message_type messageType = net_buffer_read8(&bufferReadContext);

    if (messageType == InventoryFetchResponse)
    {
        assert(readRet == 4);
    }
    else
    {
        printf("[EquipMain] Expect message type of InventoryFetchResponse but got %d\n", (int)messageType);
        exit(1);
    }

    const uint8_t* fetchedStockInfo = net_buffer_get_current(&bufferReadContext);

    for (int i = 0; i < 3; i++)
    {
        stockInfo[i] = fetchedStockInfo[i];
    }

    return 1;
}

int send_sales_request(equip_main* equipMain, const uint8_t* salesInfo, uint8_t* newStockInfo)
{
    const int readFd = equipMain->communicationChannel.readFd;
    const int writeFd = equipMain->communicationChannel.writeFd;

    uint8_t buf[32];

    net_buffer_context bufferWriteContext;
    net_buffer_init(&bufferWriteContext, buf, NET_BUF_WRITE);

    // Wrtie message type
    net_buffer_write8(&bufferWriteContext, SalesRequest);

    // Write client id
    net_buffer_write8(&bufferWriteContext, equipMain->clientID);

    // Write sales information
    for (int i = 0; i < 3; i++)
    {
        net_buffer_write8(&bufferWriteContext, salesInfo[i]);
    }

    int payloadSize = net_buffer_write_commit(&bufferWriteContext);
    if (write(writeFd, buf, payloadSize) < 0)
    {
        printf("[EquipMain] Cannot write to pipe\n");
        exit(1);
    }

    int readRet = read(readFd, buf, sizeof(buf));

    if (readRet < 0)
    {
        printf("[EquipMain] Cannot read from pipe\n");
        exit(1);
    }

    net_buffer_context bufferReadContext;
    net_buffer_init(&bufferReadContext, buf, NET_BUF_READ);

    message_type messageType = net_buffer_read8(&bufferReadContext);

    if (messageType == SalesResponse)
    {
        assert(readRet == 5);
    }
    else
    {
        printf("[EquipMain] Expect message type of SalesReponse but got %d\n", (int)messageType);
        exit(1);
    }

    uint8_t salesResponse = net_buffer_read8(&bufferReadContext);
    const uint8_t* stockInfo = net_buffer_get_current(&bufferReadContext);

    for (int i = 0; i < 3; i++)
    {
        newStockInfo[i] = stockInfo[i];
    }

    return salesResponse;
}