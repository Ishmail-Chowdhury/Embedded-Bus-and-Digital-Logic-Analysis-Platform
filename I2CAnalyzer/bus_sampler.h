#pragma once
#include <stdint.h>
struct BusState { bool sda; bool scl; };
void initBusSampler();
BusState readBus();
void startBusCapture();
void stopBusCapture();
bool nextBusState(BusState& state);
bool busCaptureOverflowed();
