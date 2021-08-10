#pragma once

#include <stdint.h>

typedef enum _equip_info_init_mode
{
    EQUIP_INFO_CREATE,
    EQUIP_INFO_GET
} equip_info_init_mode;

typedef struct _equip_info
{
    int sharedMemID;
    uint8_t* sharedInventoryInfo;
} equip_info;

void equip_info_init(equip_info* equipInfo, equip_info_init_mode initMode);

void equip_info_read(const equip_info* equipInfo, int clientID, uint8_t* inventoryInfo);

// Return value
// -1   -> out of stock
// 0    -> ok
// 1    -> replenishment occured
int equip_info_apply_sales(const equip_info* equipInfo, int clientID, const uint8_t* salesInfo);

void equip_info_free(equip_info* equipInfo, int releaseShm);