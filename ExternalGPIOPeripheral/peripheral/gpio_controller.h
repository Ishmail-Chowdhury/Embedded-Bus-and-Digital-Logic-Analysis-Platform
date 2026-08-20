#ifndef GPIO_CONTROLLER_H
#define GPIO_CONTROLLER_H

#include <Arduino.h>
#include <Wire.h>

class GPIOController {
  public:
    GPIOController();
    void begin();

    void setGPIO0(uint8_t value);
    void setGPIO1(uint8_t value);
    uint8_t readGPIO0() const;
    uint8_t readGPIO1() const;

    void setDirection0(uint8_t mask);
    void setDirection1(uint8_t mask);
    uint8_t getDirection0() const;
    uint8_t getDirection1() const;

    void updateInputRegisters();

  private:
    uint8_t gpio0Direction_;
    uint8_t gpio1Direction_;
    uint8_t gpio0Output_;
    uint8_t gpio1Output_;

    uint8_t readPort(uint8_t address) const;
    void writePort(uint8_t address, uint8_t value) const;
};

#endif
