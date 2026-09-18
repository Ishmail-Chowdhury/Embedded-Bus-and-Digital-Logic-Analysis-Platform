#include "bit_decoder.h"
static uint8_t currentByte = 0, bitCount = 0;
static bool ready = false, acknowledged = false;
void initBitDecoder() { currentByte = bitCount = 0; ready = acknowledged = false; }
void addBit(bool bit) {
    if (ready) return;
    if (bitCount < 8) { currentByte = (currentByte << 1) | bit; ++bitCount; }
    else { acknowledged = !bit; ready = true; }
}
bool byteReady() { return ready; }
bool byteAcknowledged() { return acknowledged; }
uint8_t pendingBitCount() { return bitCount; }
uint8_t getByte() {
    const uint8_t result = currentByte;
    currentByte = bitCount = 0; ready = false;
    return result;
}
