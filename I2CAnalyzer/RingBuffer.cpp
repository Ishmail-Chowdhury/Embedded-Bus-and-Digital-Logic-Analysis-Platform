#include "ring_buffer.h"
#include <string.h>

static Packet packets[32];
static int head = 0;
static int tail = 0;
static int count = 0;

void initRingBuffer()
{
    head = 0;
    tail = 0;
    count = 0;
    memset(packets, 0, sizeof(packets));
}

void pushPacket(const Packet& packet)
{
    packets[tail] = packet;
    tail = (tail + 1) % 32;
    if (count < 32)
    {
        count++;
    }
    else
    {
        head = (head + 1) % 32;
    }
}

int packetCount()
{
    return count;
}

bool getPacket(int index, Packet& out)
{
    if (index < 0 || index >= count)
    {
        return false;
    }
    int pos = (head + index) % 32;
    out = packets[pos];
    return true;
}
