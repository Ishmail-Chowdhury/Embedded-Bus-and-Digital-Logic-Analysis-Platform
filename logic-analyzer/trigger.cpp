#include "trigger.h"

bool triggerDetected(uint8_t currentState, uint8_t previousState, const TriggerConfig& config)
{
    if (!config.enabled || config.channel > 7)
    {
        return false;
    }

    const uint8_t mask = (1 << config.channel);
    const bool current = (currentState & mask) != 0;
    const bool previous = (previousState & mask) != 0;

    if (config.risingEdge)
    {
        return (!previous && current);
    }

    return (previous && !current);
}
