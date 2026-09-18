#pragma once
#include "capture_buffer.h"
#include "trigger.h"
// Pure capture state machine, shared by the AVR sampler and native regression tests.
class CaptureSession {
public:
    void begin(uint8_t initialState, const TriggerConfig& config) {
        buffer.clear(); tick = triggerTick = remaining = 0;
        previous = initialState; trigger = config;
        triggerMask = config.channel < NUM_CHANNELS ? uint8_t(1U << config.channel) : 0;
        triggered = complete = false;
    }
    bool add(uint8_t state) {
        if (complete) return true;
        const Sample sample = {tick++, state};
        buffer.push(sample);
        if (!triggered) {
            const uint8_t mask = triggerMask;
            const bool edge = ((state ^ previous) & mask)
                && (bool(state & mask) == trigger.risingEdge);
            if (buffer.size() > PRE_TRIGGER_SAMPLES && (!trigger.enabled || edge)) {
                triggerTick = sample.tick; triggered = true;
                remaining = POST_TRIGGER_SAMPLES - 1;
                complete = remaining == 0;
            }
        } else if (--remaining == 0) complete = true;
        previous = state;
        return complete;
    }
    CaptureBuffer buffer;
    uint16_t triggerTick = 0;
private:
    TriggerConfig trigger = {0, true, true};
    uint16_t tick = 0, remaining = 0;
    uint8_t previous = 0, triggerMask = 1;
    bool triggered = false, complete = false;
};
