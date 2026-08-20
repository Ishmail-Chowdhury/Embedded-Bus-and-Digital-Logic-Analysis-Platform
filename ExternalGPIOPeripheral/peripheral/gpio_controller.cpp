#include "gpio_controller.h"

namespace {
const uint8_t PCF8574A_ADDRESS_0 = 0x20;
const uint8_t PCF8574A_ADDRESS_1 = 0x21;
}

GPIOController::GPIOController()
  : gpio0Direction_(0xFF),
    gpio1Direction_(0xFF),
    gpio0Output_(0x00),
    gpio1Output_(0x00) {
}

void GPIOController::begin() {
  Wire.begin();
  setGPIO0(0x00);
  setGPIO1(0x00);
  setDirection0(0xFF);
  setDirection1(0xFF);
}

void GPIOController::setGPIO0(uint8_t value) {
  gpio0Output_ = value;
  writePort(PCF8574A_ADDRESS_0, value);
}

void GPIOController::setGPIO1(uint8_t value) {
  gpio1Output_ = value;
  writePort(PCF8574A_ADDRESS_1, value);
}

uint8_t GPIOController::readGPIO0() const {
  return readPort(PCF8574A_ADDRESS_0);
}

uint8_t GPIOController::readGPIO1() const {
  return readPort(PCF8574A_ADDRESS_1);
}

void GPIOController::setDirection0(uint8_t mask) {
  gpio0Direction_ = mask;
}

void GPIOController::setDirection1(uint8_t mask) {
  gpio1Direction_ = mask;
}

uint8_t GPIOController::getDirection0() const {
  return gpio0Direction_;
}

uint8_t GPIOController::getDirection1() const {
  return gpio1Direction_;
}

void GPIOController::updateInputRegisters() {
  // No-op; actual input capture is done when the host reads the register map.
}

uint8_t GPIOController::readPort(uint8_t address) const {
  Wire.beginTransmission(address);
  const uint8_t result = Wire.endTransmission();
  if (result != 0) {
    return 0;
  }

  Wire.requestFrom(static_cast<int>(address), 1);
  if (Wire.available()) {
    return static_cast<uint8_t>(Wire.read());
  }

  return 0;
}

void GPIOController::writePort(uint8_t address, uint8_t value) const {
  Wire.beginTransmission(address);
  Wire.write(value);
  Wire.endTransmission();
}
