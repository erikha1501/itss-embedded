#pragma once

#include "pipe_util.h"

#include <stdint.h>

typedef struct _equip_main
{
    uint8_t clientID;

    pipe_comm_channel communicationChannel;
} equip_main;

void equip_main_init(equip_main* equipMain, uint8_t clientID);
int equip_main_establish_connection(equip_main* equipMain);