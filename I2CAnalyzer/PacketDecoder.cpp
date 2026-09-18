#include "packet_decoder.h"
#include <string.h>
static Packet currentPacket;
static bool activePacket = false, readyPacket = false;
static uint16_t selectedAddress = 0;
static bool selectedTenBit = false;
enum Phase { HEADER, LOW_ADDRESS, DATA };
static Phase phase = HEADER;
void beginPacket(uint32_t atUs) {
    memset(&currentPacket, 0, sizeof(currentPacket));
    currentPacket.startUs = atUs; currentPacket.nackIndex = 255;
    activePacket = readyPacket = false; phase = HEADER;
}
void initPacketDecoder() { selectedTenBit = false; beginPacket(0); }
void feedByte(uint8_t byte, bool acknowledged) {
    uint8_t index = 0;
    if (phase == HEADER) {
        activePacket = true;
        currentPacket.read = byte & 1;
        currentPacket.tenBit = (byte & 0xF8) == 0xF0;
        if (currentPacket.tenBit) {
            currentPacket.address = uint16_t(byte & 6) << 7;
            if (currentPacket.read) {
                if (selectedTenBit && (selectedAddress & 0x300) == currentPacket.address)
                    currentPacket.address = selectedAddress;
                else currentPacket.flags |= PACKET_ADDRESS_ERROR;
                phase = DATA;
            } else { selectedTenBit = false; phase = LOW_ADDRESS; }
        } else { currentPacket.address = byte >> 1; selectedTenBit = false; phase = DATA; }
    } else if (phase == LOW_ADDRESS) {
        currentPacket.address |= byte; phase = DATA;
        selectedTenBit = acknowledged && !(currentPacket.flags & PACKET_NACK);
        selectedAddress = currentPacket.address;
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
    if (activePacket) {
        if (phase == LOW_ADDRESS) flags |= PACKET_INCOMPLETE;
        currentPacket.flags |= flags; readyPacket = true; activePacket = false;
    }
    if (!(flags & PACKET_RESTART)) selectedTenBit = false;
}
bool packetReady() { return readyPacket; }
Packet getPacket() { Packet packet = currentPacket; beginPacket(0); return packet; }
