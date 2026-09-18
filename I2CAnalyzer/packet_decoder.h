#pragma once
#include <stdint.h>
enum PacketFlags : uint8_t {
    PACKET_NACK = 1, PACKET_TRUNCATED = 2, PACKET_RESTART = 4,
    PACKET_UNSUPPORTED_ADDRESS = 8, PACKET_INCOMPLETE = 16
};
struct Packet {
    uint8_t address;
    bool read;
    uint8_t data[16];
    uint8_t length;
    uint8_t flags;
    uint8_t nackIndex; // 0 = address, 1..16 = data, 255 = none / beyond stored bytes
};
void initPacketDecoder();
void feedByte(uint8_t byte, bool acknowledged = true);
void stopPacket(uint8_t flags = 0);
bool packetReady();
Packet getPacket();
