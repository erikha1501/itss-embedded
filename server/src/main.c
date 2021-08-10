#include <stdio.h>

#include "manager/connect.h"
#include "manager/equip_info.h"

int main()
{
    equip_info equipInfo;
    equip_info_init(&equipInfo, EQUIP_INFO_CREATE);

    connect_manager connectManager;

    connect_mgr_init(&connectManager);
    connect_mgr_start(&connectManager);

    equip_info_free(&equipInfo, 1);

    return 0;
}