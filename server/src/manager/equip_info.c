#include "equip_info.h"

#include "../constants.h"
#include <commodity_info.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <time.h>

#define SHM_KEY_FILE_PATH "database/key"
#define SHM_KEY_PROJ_ID 'a'

#define SALES_HISTORY_FILE_PATH "database/salesHistory.txt"
#define INVENTORY_INFO_FILE_PATH "database/inventoryInfo.txt"

void read_inventory_info_from_db(uint8_t* inventoryInfo);
void write_inventory_info_to_db(uint8_t* inventoryInfo);
void log_sales_history(uint8_t clientID, const uint8_t* salesInfo);

void equip_info_init(equip_info* equipInfo, equip_info_init_mode initMode)
{
    equipInfo->sharedMemID = -1;
    equipInfo->sharedInventoryInfo = NULL;

    key_t shmKey = ftok(SHM_KEY_FILE_PATH, (int)SHM_KEY_PROJ_ID);

    int shmID = -1;
    if (initMode == EQUIP_INFO_CREATE)
    {
        shmID = shmget(shmKey, MAX_CLIENT_COUNT * COMMODITY_COUNT * sizeof(uint8_t), IPC_CREAT | IPC_EXCL | 0660);
    }
    else if (initMode == EQUIP_INFO_GET)
    {
        shmID = shmget(shmKey, MAX_CLIENT_COUNT * COMMODITY_COUNT * sizeof(uint8_t), 0);
    }

    if (shmID == -1)
    {
        perror("shmget failed");
        exit(1);
    }

    // Attach
    uint8_t* mem = (uint8_t*)shmat(shmID, NULL, 0);
    if (mem == (void*)-1)
    {
        perror("shmat failed");
        exit(1);
    }


    if (initMode == EQUIP_INFO_CREATE)
    {
        read_inventory_info_from_db(mem);

        // For creation purpose, shared memory is not
        // needed after inventory info is populated
        shmdt(mem);
        mem = NULL;
    }

    equipInfo->sharedMemID = shmID;
    equipInfo->sharedInventoryInfo = mem;
}

void read_inventory_info_from_db(uint8_t* inventoryInfo)
{
    FILE* inventoryFile = fopen(INVENTORY_INFO_FILE_PATH, "r");

    if (inventoryFile == NULL)
    {
        printf("Cannot open %s\n", INVENTORY_INFO_FILE_PATH);
        exit(1);
    }

    for (int i = 0; i < MAX_CLIENT_COUNT; i++)
    {
        for (int j = 0; j < COMMODITY_COUNT; j++)
        {
            int stockValue;
            if (fscanf(inventoryFile, "%d", &stockValue) <= 0)
            {
                printf("Invalid inventory info format\n");
                exit(1);
            }

            *(inventoryInfo++) = stockValue;
        }
    }

    fclose(inventoryFile);
}

void write_inventory_info_to_db(uint8_t* inventoryInfo)
{
    FILE* inventoryFile = fopen(INVENTORY_INFO_FILE_PATH, "w");

    if (inventoryFile == NULL)
    {
        printf("Cannot open %s\n", INVENTORY_INFO_FILE_PATH);
        exit(1);
    }

    for (int i = 0; i < MAX_CLIENT_COUNT; i++)
    {
        for (int j = 0; j < COMMODITY_COUNT; j++)
        {
            int stockValue = inventoryInfo[j];
            if (fprintf(inventoryFile, "%d ", stockValue) <= 0)
            {
                printf("Cannot write to %s\n", INVENTORY_INFO_FILE_PATH);
                exit(1);
            }
        }

        fputs("\n", inventoryFile);
    }

    fclose(inventoryFile);
}

void equip_info_read(const equip_info* equipInfo, int clientID, uint8_t* inventoryInfo)
{
    uint8_t* readCursor = equipInfo->sharedInventoryInfo + clientID * COMMODITY_COUNT;

    for (int i = 0; i < COMMODITY_COUNT; i++)
    {
        *(inventoryInfo++) = *(readCursor++);
    }
}

int equip_info_apply_sales(const equip_info* equipInfo, int clientID, const uint8_t* salesInfo)
{
    uint8_t* inventoryInfo = equipInfo->sharedInventoryInfo + clientID * COMMODITY_COUNT;

    // Check for stock availability
    for (int i = 0; i < COMMODITY_COUNT; i++)
    {
        uint8_t stockValue = inventoryInfo[i];
        uint8_t saleAmount = salesInfo[i];

        if (saleAmount > stockValue)
        {
            // Out of stock
            return -1;
        }
    }

    int replenished = 0;
    for (int i = 0; i < COMMODITY_COUNT; i++)
    {
        uint8_t stockValue = inventoryInfo[i];
        uint8_t saleAmount = salesInfo[i];

        uint8_t newStockValue = stockValue - saleAmount;

        if (newStockValue < COMMODITY_STOCK_REPLENISH_THRESHOLD)
        {
            newStockValue = COMMODITY_STOCK_CAPACITY;
            replenished = 1;
        }

        inventoryInfo[i] = newStockValue;
    }

    log_sales_history(clientID, salesInfo);
    write_inventory_info_to_db(equipInfo->sharedInventoryInfo);

    return replenished;
}

void log_sales_history(uint8_t clientID, const uint8_t* salesInfo)
{
    FILE* salesHistoryFile = fopen(SALES_HISTORY_FILE_PATH, "a");

    if (salesHistoryFile == NULL)
    {
        printf("Cannot open %s\n", SALES_HISTORY_FILE_PATH);
        exit(1);
    }

    time_t rawTime = time(NULL);
    struct tm* localTime = localtime(&rawTime);

    char timeString[80];
    strftime(timeString, sizeof(timeString), "[%D-%T]", localTime);

    fputs(timeString, salesHistoryFile);
    fprintf(salesHistoryFile, "[Vending machine #%d]", clientID);

    const char* commodityNames[COMMODITY_COUNT] = {COMMODITY_NAME_1, COMMODITY_NAME_2, COMMODITY_NAME_3};

    for (int i = 0; i < COMMODITY_COUNT; i++)
    {
        uint8_t saleAmount = salesInfo[i];
        ;
        if (salesInfo > 0)
        {
            fprintf(salesHistoryFile, "%s(%d) ", commodityNames[i], (int)saleAmount);
        }
    }

    fputs("\n", salesHistoryFile);

    fclose(salesHistoryFile);
}

void equip_info_free(equip_info* equipInfo, int releaseShm)
{
    if (equipInfo->sharedInventoryInfo != NULL)
    {
        shmdt(equipInfo->sharedInventoryInfo);
        equipInfo->sharedInventoryInfo = NULL;
    }

    if (releaseShm)
    {
        shmctl(equipInfo->sharedMemID, IPC_RMID, NULL);
    }
}