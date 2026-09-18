#include "ring_buffer.h"
#include <string.h>
#include "config.h"

static Packet packets[PACKET_HISTORY_SIZE];
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
    tail = (tail + 1) % PACKET_HISTORY_SIZE;
    if (count < PACKET_HISTORY_SIZE)
    {
        count++;
    }
    else
    {
        head = (head + 1) % PACKET_HISTORY_SIZE;
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
    int pos = (head + index) % PACKET_HISTORY_SIZE;
    out = packets[pos];
    return true;
}
