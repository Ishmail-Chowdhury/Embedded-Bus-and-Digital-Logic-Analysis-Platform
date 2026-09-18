#pragma once
#include <Arduino.h>
#include "expander_bus.h"

class GPIOController {
public:
    GPIOController();
    void begin();
    // Only called from loop(), never from a Wire callback.
    bool apply(uint8_t bank, uint8_t output, uint8_t direction, bool enabled);
    bool read(uint8_t bank, uint8_t& value);
    static uint8_t portValue(uint8_t output, uint8_t direction, bool enabled) {
        return enabled ? (output | direction) : 0xFF;
    }
private:
    ExpanderBus bus_;
    uint8_t applied_[2];
    bool valid_[2];
};
