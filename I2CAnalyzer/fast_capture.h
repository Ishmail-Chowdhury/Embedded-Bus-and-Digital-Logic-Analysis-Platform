#pragma once
#include "bus_sampler.h"
static const uint16_t FAST_CAPTURE_BYTES = 512;
// Two simultaneous A4/A5 observations per byte, one observation per microsecond.
// Even samples occupy bits 5:4; odd samples occupy bits 1:0.
inline BusState fastBusState(const uint8_t* bytes, uint16_t index) {
    const uint8_t bits = bytes[index / 2] >> ((index & 1) ? 0 : 4);
    return BusState(bits & 1, bits & 2, index);
}
uint16_t captureFastBus(uint8_t* bytes, uint16_t capacity);
void decodeFastBus(const uint8_t* bytes, uint16_t samples);
