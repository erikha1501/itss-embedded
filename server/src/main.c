#include <stdio.h>

#include "manager/connect.h"
#include "manager/equip_info.h"
#include <commodity_info.h>

void print_inventory_info(const uint8_t* inventoryInfo);

int main()
{
    equip_info equipInfo;
    equip_info_init(&equipInfo, EQUIP_INFO_CREATE);
    print_inventory_info(equipInfo.sharedInventoryInfo);

    connect_manager connectManager;

    connect_mgr_init(&connectManager);
    connect_mgr_start(&connectManager);

    equip_info_free(&equipInfo, 1);

    return 0;
}

void print_inventory_info(const uint8_t* inventoryInfo)
{
    printf("Loading inventory info from database...\n");

    for (int i = 0; i < MAX_CLIENT_COUNT * COMMODITY_COUNT; i++)
    {
        uint8_t stockValue = *(inventoryInfo++);
        printf("%d ", (int)stockValue);
    }

    printf("\n");
}