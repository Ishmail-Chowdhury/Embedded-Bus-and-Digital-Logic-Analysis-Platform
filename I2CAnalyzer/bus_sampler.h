#pragma once
#include <stdint.h>
struct BusState {
    bool sda, scl;
    uint32_t atUs;
    BusState(bool data = true, bool clock = true, uint32_t time = 0) : sda(data), scl(clock), atUs(time) {}
};
void initBusSampler();
BusState readBus();
void startBusCapture();
void stopBusCapture();
bool nextBusState(BusState& state);
bool busCaptureOverflowed();
bool busCaptureIdleTime(uint32_t& atUs);
