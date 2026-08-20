# Embedded Bus and Digital Logic Analysis Platform

## Project Review Summary

This repository is a multi-mode Arduino platform focused on embedded bus observation, digital waveform capture, and programmable peripheral emulation.

It contains three working modules:

1. Mode 1: I2C Analyzer
2. Mode 2: Logic Analyzer
3. Mode 3: External GPIO Peripheral

Each mode is implemented as a separate sketch and can be developed or tested independently.

## Repository Structure

- `I2CAnalyzer/` -> I2C transaction decoding module
- `logic-analyzer/` -> 8-channel logic capture module
- `ExternalGPIOPeripheral/` -> host/peripheral I2C device emulation module

## Overall Technical State

- Clean module separation by purpose
- Uno-oriented memory-aware implementation choices
- Good educational value for embedded bus analysis and register-mapped interface design

---

## Mode 1: I2C Analyzer

### Purpose

Decode I2C transactions and show packet history on an OLED while filtering internal display traffic.

### Entry Sketch

- `I2CAnalyzer/I2CAnalyzer.ino`

### Required Libraries

- Adafruit GFX Library
- Adafruit SSD1306

Install from Arduino IDE:

`Sketch -> Include Library -> Manage Libraries...`

### Wiring (Elegoo Uno R3)

- OLED VCC -> 5V
- OLED GND -> GND
- OLED SDA -> A4
- OLED SCL -> A5
- Next button -> D2 (INPUT_PULLUP, button to GND)
- Prev button -> D3 (INPUT_PULLUP, button to GND)

### Behavior Notes

- Uses A4/A5 bus sampling
- Filters OLED traffic at address `0x3C` before packet history storage
- Ring buffer stores latest 32 decoded packets
- Packet browsing is controlled by next/prev buttons

### Upload Steps

1. Open `I2CAnalyzer/I2CAnalyzer.ino` in Arduino IDE.
2. Select board: Arduino Uno.
3. Select the correct serial port.
4. Upload.
5. Open Serial Monitor at 115200 baud for debug output.

---

## Mode 2: Logic Analyzer

### Purpose

Standalone 8-channel digital logic analyzer for live monitoring and triggered waveform capture.

### Entry Sketch

- `logic-analyzer/logic-analyzer.ino`

### Phase Coverage

- Phase 1: live 8-channel state monitor
- Phase 2: timestamped capture with fixed interval
- Phase 3: edge trigger plus ring buffer plus pre/post trigger capture

### Required Library

- U8g2 by oliver

The sketch uses U8x8 mode from U8g2 to reduce RAM usage on Uno.

### Wiring (Elegoo Uno R3)

Analyzer inputs:

- CH0 -> D2
- CH1 -> D3
- CH2 -> D4
- CH3 -> D5
- CH4 -> D6
- CH5 -> D7
- CH6 -> A0
- CH7 -> A1

OLED (software I2C):

- OLED VCC -> 5V
- OLED GND -> GND
- OLED SDA -> D8
- OLED SCL -> D9

### Sampling and Buffer

- Sample interval: 10 us (100 kS/s)
- Buffer size: 512 samples
- Trigger edge: rising or falling selectable
- Trigger channel: CH0 to CH7 selectable

Each sample stores:

- `tick` as `uint16_t`
- `state` as `uint8_t`

Microseconds are derived as: `tick * SAMPLE_INTERVAL_US`.

### Serial Commands (115200)

- `l` = live monitor mode
- `a` = arm capture without clearing buffer
- `r` = re-arm and clear buffer
- `t` = toggle trigger edge (rising or falling)
- `e` = enable or disable trigger
- `c0` to `c7` = set trigger channel
- `n` = next sample view (after capture)
- `p` = previous sample view (after capture)

### Quick Start

1. Open `logic-analyzer/logic-analyzer.ino` in Arduino IDE.
2. Select board: Arduino Uno.
3. Install U8g2 if needed.
4. Upload.
5. Open Serial Monitor at 115200 baud.
6. Run `l` to check live channel states.
7. Run `r` to arm trigger capture, then inspect data with `n` and `p`.

---

## Mode 3: External GPIO Peripheral

### Purpose

Implements a software-defined, register-mapped I2C peripheral using Uno boards and PCF8574A GPIO expanders.

### Architecture Model

- Host Uno acts as controller
- Peripheral Uno acts as external device
- Host communicates through I2C register read/write operations
- Peripheral exposes register map and drives external GPIO state

### Project Structure

- `ExternalGPIOPeripheral/peripheral/peripheral.ino`
- `ExternalGPIOPeripheral/peripheral/config.h`
- `ExternalGPIOPeripheral/peripheral/gpio_controller.h`
- `ExternalGPIOPeripheral/peripheral/gpio_controller.cpp`
- `ExternalGPIOPeripheral/peripheral/registers.h`
- `ExternalGPIOPeripheral/peripheral/registers.cpp`
- `ExternalGPIOPeripheral/peripheral/interrupt_controller.h`
- `ExternalGPIOPeripheral/peripheral/interrupt_controller.cpp`
- `ExternalGPIOPeripheral/host/host.ino`

### Register Map

- `0x00` REG_GPIO0_OUTPUT
- `0x01` REG_GPIO1_OUTPUT
- `0x02` REG_GPIO0_INPUT
- `0x03` REG_GPIO1_INPUT
- `0x04` REG_DIRECTION0
- `0x05` REG_DIRECTION1
- `0x06` REG_STATUS
- `0x07` REG_CONTROL
- `0x08` REG_INTERRUPT_STATUS
- `0x09` REG_INTERRUPT_ENABLE
- `0x0A` REG_DEVICE_ID

### Example Transactions

- Write `0x55` to register `0x00` to drive output bits
- Read register `0x02` to get GPIO0 input state

### Phase Goals

- Phase 1: basic register-mapped GPIO read/write
- Phase 2: direction control, status, and device identity
- Phase 3: interrupt-driven behavior for switch/button changes

### Why This Module Matters

This module demonstrates practical embedded design patterns:

- I2C slave implementation
- Register-mapped peripheral interfaces
- Hardware abstraction through software-defined registers
- Verification workflows using the I2C and logic analyzer modules

---

## Shared Setup Notes

### Arduino IDE Workflow

1. Open the target sketch.
2. Select board: Arduino Uno.
3. Select the correct serial port.
4. Upload.
5. Use Serial Monitor at 115200 baud when serial control/debug is needed.

### Library Summary

- I2C Analyzer: Adafruit GFX, Adafruit SSD1306
- Logic Analyzer: U8g2

## Documentation Direction

This root README now serves as the primary project document with consolidated module information.
