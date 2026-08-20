#include "registers.h"
#include "interrupt_controller.h"

RegisterMap::RegisterMap(GPIOController& gpioController, InterruptController& interruptController)
  : gpioController_(gpioController),
    interruptController_(interruptController) {
  memset(registers_, 0, sizeof(registers_));
  registers_[REG_DEVICE_ID] = DEVICE_ID_VALUE;
}

void RegisterMap::begin() {
  registers_[REG_DEVICE_ID] = DEVICE_ID_VALUE;
  registers_[REG_STATUS] = STATUS_ENABLED;
  registers_[REG_CONTROL] = 0x00;
  registers_[REG_INTERRUPT_ENABLE] = 0x00;
  registers_[REG_INTERRUPT_STATUS] = 0x00;
}

void RegisterMap::writeRegister(uint8_t address, uint8_t value) {
  if (address >= REGISTER_COUNT) {
    return;
  }

  switch (address) {
    case REG_GPIO0_OUTPUT:
      gpioController_.setGPIO0(value);
      setRegister(address, value);
      break;

    case REG_GPIO1_OUTPUT:
      gpioController_.setGPIO1(value);
      setRegister(address, value);
      break;

    case REG_DIRECTION0:
      gpioController_.setDirection0(value);
      setRegister(address, value);
      break;

    case REG_DIRECTION1:
      gpioController_.setDirection1(value);
      setRegister(address, value);
      break;

    case REG_CONTROL:
      setRegister(address, value);
      if ((value & 0x01) == 0x01) {
        registers_[REG_STATUS] |= STATUS_ENABLED;
      }
      break;

    case REG_INTERRUPT_ENABLE:
      setRegister(address, value);
      break;

    default:
      setRegister(address, value);
      break;
  }

  updateStatusFromInputs();
}

uint8_t RegisterMap::readRegister(uint8_t address) const {
  if (address >= REGISTER_COUNT) {
    return 0;
  }

  switch (address) {
    case REG_GPIO0_INPUT:
      return gpioController_.readGPIO0();

    case REG_GPIO1_INPUT:
      return gpioController_.readGPIO1();

    case REG_DIRECTION0:
      return gpioController_.getDirection0();

    case REG_DIRECTION1:
      return gpioController_.getDirection1();

    case REG_INTERRUPT_STATUS:
      return registers_[REG_INTERRUPT_STATUS];

    case REG_DEVICE_ID:
      return registers_[REG_DEVICE_ID];

    default:
      return getRegister(address);
  }
}

void RegisterMap::updateStatusFromInputs() {
  uint8_t input0 = gpioController_.readGPIO0();
  uint8_t input1 = gpioController_.readGPIO1();

  registers_[REG_GPIO0_INPUT] = input0;
  registers_[REG_GPIO1_INPUT] = input1;

  if (interruptController_.hasPendingInterrupt()) {
    registers_[REG_STATUS] |= STATUS_INTERRUPT;
  } else {
    registers_[REG_STATUS] &= static_cast<uint8_t>(~STATUS_INTERRUPT);
  }
}

void RegisterMap::setRegister(uint8_t address, uint8_t value) {
  if (address < REGISTER_COUNT) {
    registers_[address] = value;
  }
}

uint8_t RegisterMap::getRegister(uint8_t address) const {
  if (address < REGISTER_COUNT) {
    return registers_[address];
  }

  return 0;
}
