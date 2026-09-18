#pragma once
#include <Arduino.h>
#include "config.h"
#include "gpio_controller.h"
class InterruptController;
class RegisterMap {
public:
    RegisterMap(GPIOController& gpio, InterruptController& interrupt);
    void begin();
    // ISR-safe: cached bytes only, no I2C, delays, heap, or Serial.
    void writeRegister(uint8_t address, uint8_t value);
    uint8_t readRegister(uint8_t address) const;
    void updateStatusFromInputs();
    void snapshot(uint8_t address, uint8_t* out, uint8_t length) const;
private:
    GPIOController& gpioController_;
    InterruptController& interruptController_;
    volatile uint8_t registers_[REGISTER_COUNT];
    uint8_t candidate_[2];
    uint32_t changedAt_[16];
    volatile uint16_t revision_ = 0;
    uint16_t sampledRevision_[2];
    bool inputValid_[2];
    void syncInterrupt();
};
