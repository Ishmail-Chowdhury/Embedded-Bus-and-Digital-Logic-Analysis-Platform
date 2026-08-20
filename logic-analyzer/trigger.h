#pragma once

#include <Arduino.h>

struct TriggerConfig
{
    uint8_t channel;
    bool risingEdge;
    bool enabled;
};

bool triggerDetected(uint8_t currentState, uint8_t previousState, const TriggerConfig& config);
