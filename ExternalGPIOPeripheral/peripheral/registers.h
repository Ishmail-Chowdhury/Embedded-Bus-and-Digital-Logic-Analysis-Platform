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
private:
    GPIOController& gpioController_;
    InterruptController& interruptController_;
    volatile uint8_t registers_[REGISTER_COUNT];
    uint8_t lastInput_[2];
    uint8_t lastDirection_[2];
    bool inputValid_[2];
    void syncInterrupt();
};
