#pragma once

#include <stdint.h>

struct Packet
{
    uint8_t address;
    bool read;
    uint8_t data[16];
    uint8_t length;
};

void initPacketDecoder();
void feedByte(uint8_t byte);
void stopPacket();
bool packetReady();
Packet getPacket();

