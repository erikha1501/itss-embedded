#pragma once

#include <stdint.h>

typedef enum _net_buffer_mode
{
    NET_BUF_READ,   // Start reading from offset 0
    NET_BUF_WRITE   // Start writing from offset 4
} net_buffer_mode;

typedef struct _net_buffer_context
{
    uint8_t* buffer;
    uint8_t* cursor;
} net_buffer_context;

void net_buffer_init(net_buffer_context* bufferContext, uint8_t* buffer, net_buffer_mode mode);

void net_buffer_write8(net_buffer_context* bufferContext, uint8_t value);
void net_buffer_write16(net_buffer_context* bufferContext, uint16_t value);
void net_buffer_write32(net_buffer_context* bufferContext, uint32_t value);

// Write message length header
// Return the number of bytes written (including header)
int net_buffer_write_commit(net_buffer_context* bufferContext);

uint8_t net_buffer_read8(net_buffer_context* bufferContext);
uint16_t net_buffer_read16(net_buffer_context* bufferContext);
uint32_t net_buffer_read32(net_buffer_context* bufferContext);

uint8_t* net_buffer_get_current(net_buffer_context* bufferContext);