#include "edge_detector.h"

static bool previousSDA = true;
static bool previousSCL = true;

void initEdgeDetector()
{
    previousSDA = true;
    previousSCL = true;
}

Event detectEdge(const BusState& state)
{
    Event event = NONE;

    if (previousSDA && !state.sda && state.scl)
    {
        event = START;
    }
    else if (!previousSDA && state.sda && state.scl)
    {
        event = STOP;
    }
    else if (!previousSCL && state.scl)
    {
        event = CLOCK_RISE;
    }
    else if (previousSCL && !state.scl)
    {
        event = CLOCK_FALL;
    }

    previousSDA = state.sda;
    previousSCL = state.scl;
    return event;
}
