#include "../inc/net_buffer.h"

#include <arpa/inet.h>
#include <assert.h>

void net_buffer_init(net_buffer_context* bufferContext, uint8_t* buffer, net_buffer_mode mode)
{
    bufferContext->buffer = buffer;

    if (mode == NET_BUF_WRITE)
    {
        bufferContext->cursor = buffer + sizeof(uint32_t);
    }
    else
    {
        bufferContext->cursor = buffer;
    }
}

void net_buffer_write8(net_buffer_context* bufferContext, uint8_t value)
{
    uint8_t* pValue = (uint8_t*)bufferContext->cursor;
    *pValue = value;

    bufferContext->cursor += sizeof(uint8_t);
}

void net_buffer_write16(net_buffer_context* bufferContext, uint16_t value)
{
    uint16_t* pValue = (uint16_t*)bufferContext->cursor;
    *pValue = htons(value);

    bufferContext->cursor += sizeof(uint16_t);
}

void net_buffer_write32(net_buffer_context* bufferContext, uint32_t value)
{
    uint32_t* pValue = (uint32_t*)bufferContext->cursor;
    *pValue = htonl(value);

    bufferContext->cursor += sizeof(uint32_t);
}

int net_buffer_write_commit(net_buffer_context* bufferContext)
{
    uint32_t* pMessageLength = (uint32_t*)bufferContext->buffer;
    uint32_t messageLength = bufferContext->cursor - (bufferContext->buffer + 4);

    *pMessageLength = htonl(messageLength);

    return bufferContext->cursor - bufferContext->buffer;
}

uint8_t net_buffer_read8(net_buffer_context* bufferContext)
{
    uint8_t* pValue = (uint8_t*)bufferContext->cursor;

    bufferContext->cursor += sizeof(uint8_t);

    return *pValue;
}

uint16_t net_buffer_read16(net_buffer_context* bufferContext)
{
    uint16_t* pValue = (uint16_t*)bufferContext->cursor;

    bufferContext->cursor += sizeof(uint16_t);

    return ntohs(*pValue);
}

uint32_t net_buffer_read32(net_buffer_context* bufferContext)
{
    uint32_t* pValue = (uint32_t*)bufferContext->cursor;

    bufferContext->cursor += sizeof(uint32_t);

    return ntohl(*pValue);
}

uint8_t* net_buffer_get_current(net_buffer_context* bufferContext)
{
    return bufferContext->cursor;
}