# External GPIO Peripheral

This project reimagines the Uno + PCF8574 board as a programmable external peripheral that behaves like a hardware register-mapped device.

The design follows a host/peripheral model:

- Host UNO acts as the controller or processor
- Peripheral UNO behaves like a hardware device on the I2C bus
- The peripheral exposes a software-defined register interface
- PCF8574A expanders provide the physical GPIO resources
- LEDs, switches, and buttons are used as the visible device state

## Architecture

```text
Host UNO
  |
  | I2C register transactions
  v
Peripheral UNO
  |
  +--> Command Decoder
  +--> Register Map
  +--> GPIO Controller
  +--> PCF8574A #1 / #2
  +--> Interrupt Controller
      |
      +--> LEDs
      +--> Switches
      +--> Buttons
```

## Register map

```text
0x00  REG_GPIO0_OUTPUT     Output bits 0-7
0x01  REG_GPIO1_OUTPUT     Output bits 8-15
0x02  REG_GPIO0_INPUT      Input bits 0-7
0x03  REG_GPIO1_INPUT      Input bits 8-15
0x04  REG_DIRECTION0       Direction mask for GPIO0
0x05  REG_DIRECTION1       Direction mask for GPIO1
0x06  REG_STATUS           Peripheral status
0x07  REG_CONTROL          Configuration bits
0x08  REG_INTERRUPT_STATUS Interrupt flags
0x09  REG_INTERRUPT_ENABLE Interrupt mask
0x0A  REG_DEVICE_ID        Peripheral identifier
```

## Example transaction

The host writes a byte to register 0x00:

```text
WRITE 0x00 = 0x55
```

The peripheral updates the output register and drives the onboard LEDs using the PCF8574 outputs.

Reading inputs:

```text
READ 0x02
```

returns the current switch state from GPIO0.

## Project structure

```text
ExternalGPIOPeripheral/
├── README.md
├── peripheral/
│   ├── config.h
│   ├── gpio_controller.h
│   ├── gpio_controller.cpp
│   ├── registers.h
│   ├── registers.cpp
│   ├── interrupt_controller.h
│   ├── interrupt_controller.cpp
│   └── peripheral.ino
└── host/
    └── host.ino
```

## Phase goals

### Phase 1: Basic register-mapped GPIO
- write output registers
- read switch inputs
- turn LEDs on and off over I2C

### Phase 2: Status and configuration
- direction control bits
- status register
- device ID register

### Phase 3: Interrupt-driven peripheral behavior
- create interrupt events for button/switch changes
- host polls or reacts to an INT line

## Why this matters

This is not just GPIO expansion. It is an actual software-defined peripheral interface that looks much closer to how real embedded systems communicate with external hardware.

The host talks to the board using defined registers instead of directly toggling raw pins.

This makes the project a strong demonstration of:

- I2C slave design
- register-mapped peripheral design
- digital hardware abstraction
- embedded-system communication patterns
- verification using the logic analyzer and I2C analyzer
