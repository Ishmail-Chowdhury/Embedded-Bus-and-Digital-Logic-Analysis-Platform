#pragma once

#include "bus_sampler.h"

enum Event
{
    NONE,
    START,
    STOP,
    CLOCK_RISE,
    CLOCK_FALL
};

void initEdgeDetector();
Event detectEdge(const BusState& state);
