#pragma once

#include "equip_info.h"

#include "../constants.h"
#include <socket_reader.h>

typedef struct _sales_manager
{
    int clientSocketFd;

    socket_reader* socketReader;
    equip_info equipInfo;
} sales_manager;

void sales_mgr_init(sales_manager* salesManager, int clientSocketFd);
void sales_mgr_start(sales_manager* salesManager);

void sales_mgr_free(sales_manager* salesManager);