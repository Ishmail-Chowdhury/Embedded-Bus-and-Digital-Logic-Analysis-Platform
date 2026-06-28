#include "packet_decoder.h"
#include <string.h>

static Packet currentPacket;
static bool activePacket = false;
static bool readyPacket = false;

void initPacketDecoder()
{
    memset(&currentPacket, 0, sizeof(currentPacket));
    activePacket = false;
    readyPacket = false;
}

void feedByte(uint8_t byte)
{
    if (!activePacket)
    {
        memset(&currentPacket, 0, sizeof(currentPacket));
        currentPacket.address = byte >> 1;
        currentPacket.read = (byte & 0x01) != 0;
        currentPacket.length = 0;
        activePacket = true;
        readyPacket = false;
        return;
    }

    if (currentPacket.length < sizeof(currentPacket.data))
    {
        currentPacket.data[currentPacket.length++] = byte;
    }
}

bool packetReady()
{
    return readyPacket;
}

Packet getPacket()
{
    Packet packet = currentPacket;
    readyPacket = false;
    activePacket = false;
    memset(&currentPacket, 0, sizeof(currentPacket));
    return packet;
}

void stopPacket()
{
    if (activePacket)
    {
        readyPacket = true;
        activePacket = false;
    }
}
