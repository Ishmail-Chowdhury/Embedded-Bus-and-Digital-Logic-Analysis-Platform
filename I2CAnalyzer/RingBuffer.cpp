#include "ring_buffer.h"
#include <string.h>
#include "config.h"
#include "fast_capture.h"

static union {
    Packet packets[PACKET_HISTORY_SIZE];
    uint8_t raw[FAST_CAPTURE_BYTES];
} storage;
static int head = 0;
static int tail = 0;
static int count = 0;

void initRingBuffer()
{
    head = 0;
    tail = 0;
    count = 0;
    memset(&storage, 0, sizeof(storage));
}
uint8_t* packetScratch() { initRingBuffer(); return storage.raw; }
const uint8_t* rawBusCapture() { return storage.raw; }

void pushPacket(const Packet& packet)
{
    storage.packets[tail] = packet;
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
    out = storage.packets[pos];
    return true;
}
