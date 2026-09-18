#include "packet_decoder.h"
#include <string.h>
static Packet currentPacket;
static bool activePacket = false, readyPacket = false;
void initPacketDecoder() {
    memset(&currentPacket, 0, sizeof(currentPacket));
    currentPacket.nackIndex = 255;
    activePacket = readyPacket = false;
}
void feedByte(uint8_t byte, bool acknowledged) {
    uint8_t index = 0;
    if (!activePacket) {
        initPacketDecoder();
        currentPacket.address = byte >> 1;
        currentPacket.read = byte & 1;
        if ((byte & 0xF8) == 0xF0) currentPacket.flags |= PACKET_UNSUPPORTED_ADDRESS;
        activePacket = true;
    } else if (currentPacket.length < sizeof(currentPacket.data)) {
        currentPacket.data[currentPacket.length++] = byte;
        index = currentPacket.length;
    } else { currentPacket.flags |= PACKET_TRUNCATED; index = 255; }
    if (!acknowledged) {
        if (!(currentPacket.flags & PACKET_NACK)) currentPacket.nackIndex = index;
        currentPacket.flags |= PACKET_NACK;
    }
}
void stopPacket(uint8_t flags) {
    if (activePacket) { currentPacket.flags |= flags; readyPacket = true; activePacket = false; }
}
bool packetReady() { return readyPacket; }
Packet getPacket() { Packet packet = currentPacket; initPacketDecoder(); return packet; }
