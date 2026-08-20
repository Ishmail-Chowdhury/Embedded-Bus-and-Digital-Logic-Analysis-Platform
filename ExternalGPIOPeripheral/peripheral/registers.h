#ifndef REGISTERS_H
#define REGISTERS_H

#include <Arduino.h>
#include "config.h"
#include "gpio_controller.h"

class InterruptController;

class RegisterMap {
  public:
    RegisterMap(GPIOController& gpioController, InterruptController& interruptController);
    void begin();
    void writeRegister(uint8_t address, uint8_t value);
    uint8_t readRegister(uint8_t address) const;
    void updateStatusFromInputs();

  private:
    GPIOController& gpioController_;
    InterruptController& interruptController_;
    uint8_t registers_[REGISTER_COUNT];

    void setRegister(uint8_t address, uint8_t value);
    uint8_t getRegister(uint8_t address) const;
};

#endif
