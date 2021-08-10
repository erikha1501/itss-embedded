#pragma once

typedef enum _message_type
{
    ConnectRequest,
    ConnectResponse,

    InventoryFetchRequest,
    InventoryFetchResponse,

    SalesRequest,
    SalesResponse
} message_type;