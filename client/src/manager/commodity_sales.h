#pragma once

#include "pipe_util.h"

#include <stdint.h>

typedef struct _commodity_sales
{
    int clientSocketFd;

    int equipMainPipeReadFd;
    int equipMainPipeWriteFd;

    pipe_comm_channel communicationChannel;
} commodity_sales;

void commodity_sales_init(commodity_sales* commoditySales, int clientSocketFd, pipe_comm_channel communicationChannel);
void commodity_sales_start(commodity_sales* commoditySales);