# Logic Analyzer (Mode 2)

This sketch is a standalone 8-channel digital logic analyzer, separate from the I2C analyzer.

## Phase Coverage

- Phase 1: live 8-channel state monitor
- Phase 2: timestamped capture (sample tick + fixed interval)
- Phase 3: edge trigger + ring buffer + pre/post trigger capture

## Folder

- `logic-analyzer/logic-analyzer.ino`
- `logic-analyzer/config.h`
- `logic-analyzer/sampler.h`
- `logic-analyzer/sampler.cpp`
- `logic-analyzer/trigger.h`
- `logic-analyzer/trigger.cpp`
- `logic-analyzer/capture_buffer.h`
- `logic-analyzer/capture_buffer.cpp`
- `logic-analyzer/display.h`
- `logic-analyzer/display.cpp`

## Required Library

Install from Arduino Library Manager:

- U8g2 by oliver

The sketch uses `U8x8` mode from U8g2 for low RAM usage on Uno.

## Wiring

### Analyzer Inputs

- CH0 -> D2
- CH1 -> D3
- CH2 -> D4
- CH3 -> D5
- CH4 -> D6
- CH5 -> D7
- CH6 -> A0
- CH7 -> A1

### OLED (Software I2C)

- OLED VCC -> 5V
- OLED GND -> GND
- OLED SDA -> D8
- OLED SCL -> D9

## Sampling and Buffer

- Sample interval: `10 us` (`100 kS/s`)
- Buffer size: `512 samples`
- Trigger edge: rising/falling selectable
- Trigger channel: CH0..CH7 selectable

Because Uno RAM is limited, each sample stores:

- `tick` (`uint16_t`) instead of full `uint32_t micros`
- `state` (`uint8_t`)

Time in microseconds is calculated as `tick * SAMPLE_INTERVAL_US`.

## Serial Commands (115200)

- `l` = live monitor mode
- `a` = arm capture without clearing buffer
- `r` = re-arm and clear buffer
- `t` = toggle trigger edge (rising/falling)
- `e` = enable/disable trigger
- `c0`..`c7` = set trigger channel
- `n` = next sample view (capture complete)
- `p` = previous sample view (capture complete)

## Quick Start

1. Open `logic-analyzer/logic-analyzer.ino` in Arduino IDE.
2. Select board: Arduino Uno.
3. Install U8g2 library if not already installed.
4. Upload the sketch.
5. Open Serial Monitor at 115200 baud.
6. Use `l` to confirm live channel states.
7. Use `r` to arm trigger capture and inspect results with `n`/`p`.
