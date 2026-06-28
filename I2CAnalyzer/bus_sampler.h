#pragma once

#include <stdint.h>

struct BusState
{
    bool sda;
    bool scl;
};

void initBusSampler();
BusState readBus();
