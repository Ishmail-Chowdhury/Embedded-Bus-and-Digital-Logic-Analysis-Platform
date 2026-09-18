#pragma once
#include <Arduino.h>

// Separate open-drain bus. Never uses the upstream Wire target interface.
class ExpanderBus {
public:
    void begin();
    bool writePort(uint8_t address, uint8_t value);
    bool readPort(uint8_t address, uint8_t& value);
private:
    bool start();
    bool stop();
    bool clockHigh();
    bool writeByte(uint8_t value);
};
