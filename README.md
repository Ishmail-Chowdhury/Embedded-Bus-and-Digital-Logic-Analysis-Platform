# Embedded Bus and Digital Logic Analysis Platform

I built this platform around three independent Arduino Uno R3 modes: I2C transaction observation, eight-channel logic capture, and a register-mapped GPIO peripheral. Each sketch runs on its own 16 MHz ATmega328P board; the GPIO example uses a host and a peripheral board.

I have confirmed that physical operation is working as expected on my hardware. I also checked the firmware against component documentation, compiled all four sketches for the Uno, and ran the native regression suites. I record the functional result, build results, and repeatable test procedure in [validation](docs/validation.md), with component references and operating limits in [parts and constraints](docs/parts-and-constraints.md).

## Design and operating limits

- I keep each analyzer's OLED on a separate software-I2C connection: **SDA D8, SCL D9**. The I2C analyzer observes A4/A5 without adding display traffic to that bus.
- I connect the peripheral's expanders through **SDA D8, SCL D9**, on a bus separate from the host A4/A5 link. These pins are on a different board from the analyzer OLED.
- I configure the **PCF8574A addresses as 0x38 and 0x39** with the straps below.
- I separate I2C capture from packet browsing. OLED/Serial packet rendering happens when paused. I use a conservative software-sniffer target of **10 kHz or slower**, with SCL high/low each at least 50 µs.
- I configure logic capture as a nominal **100 kS/s burst**, with a one-second maximum arm/capture window. Every arm starts fresh; `a` and `r` are aliases. Display updates and serial commands pause during the burst.

## Build and upload

Use Arduino AVR Boards and select **Arduino Uno**. Install **U8g2** for both analyzer sketches (U8x8 text mode); Adafruit GFX/SSD1306 are no longer needed. Wire is supplied by the AVR core. I validated the builds with AVR core 1.8.8 and U8g2 2.36.18.

Open and upload one of:

- `I2CAnalyzer/I2CAnalyzer.ino`
- `logic-analyzer/logic-analyzer.ino`
- `ExternalGPIOPeripheral/peripheral/peripheral.ino`
- `ExternalGPIOPeripheral/host/host.ino`

Use 115200 baud for the analyzers and host. The peripheral runs without serial logging. Set `OLED_HEIGHT` and `OLED_ADDRESS` in the relevant analyzer's `config.h` for the actual module. My firmware defaults are: I2C analyzer 128×32, logic analyzer 128×64, address 0x3C. Both heights are supported and compiled. OLED absence does not prevent serial capture, but the display driver does not diagnose a disconnected module.

## Mode 1: I2C analyzer

| Connection | Uno pin |
|---|---|
| Observed SDA / SCL | A4 / A5 (inputs, no internal pull-ups) |
| OLED SDA / SCL | D8 / D9 |
| Next / previous button | D2 / D3, each button to GND |
| Common signal reference | GND |

Power the OLED according to its **module** specification; see the electrical constraints before connecting it to 5 V logic.

I use pin-change interrupts to queue SDA/SCL snapshots from one port read. The decoder records 7-bit addresses, direction, up to 16 data bytes, NACK position, truncation, and repeated START boundaries. History holds the latest 32 address phases; a repeated START ends one phase and starts another. Traffic to 0x3C on the observed bus is retained because the OLED is physically separate.

Commands:

| Command | Action |
|---|---|
| `r` | Clear history and arm while the observed bus is idle |
| `s` or either button | Pause; discard an unfinished address phase |
| `n` / `p` or buttons | Browse captured phases while paused |

Start with the traffic source stopped, send `r`, then start the source. After `s`, send `n` or `p` to display the selected packet. OLED shows the first eight data bytes; Serial prints all sixteen stored bytes. Arming while SDA/SCL is low is refused; wait for idle and retry.

Packet flags are hexadecimal: `01` NACK, `02` payload truncated, `04` ended by repeated START, `08` unsupported 10-bit address header, `10` incomplete byte. `NAK 0` means address NACK, `1..16` identifies a stored data byte, and `255` means none or beyond stored payload (check the NACK flag). A final read-byte NACK is normally intentional.

Queue overflow stops capture and reports edge loss. Completed phases before the loss remain available. This detects queue exhaustion; it cannot detect every transition that hardware missed at excessive bus speed. This is an educational sniffer, without 10-bit-address decoding, glitch filtering, packet timestamps, or a guaranteed 100/400 kHz capture rate.

## Mode 2: logic analyzer

| Channel | CH0 | CH1 | CH2 | CH3 | CH4 | CH5 | CH6 | CH7 |
|---|---|---|---|---|---|---|---|---|
| Uno pin | D2 | D3 | D4 | D5 | D6 | D7 | A0 | A1 |

OLED uses D8/D9 and GND. Inputs are high impedance; provide defined external levels. See [input voltage and loading constraints](docs/parts-and-constraints.md).

I use Timer1 to schedule nominal 10 µs intervals. The 512-byte state ring reconstructs 16-bit sample ticks, including rollover, instead of storing redundant timestamps. A complete capture contains **100 samples before the trigger and 412 from the trigger onward**: one trigger sample plus 411 subsequent samples. Trigger edges before 100 samples of history are ignored. Relative times range from −1000 to +4110 µs; first-to-last span is 5.11 ms.

| Command | Action |
|---|---|
| `l` | Live state display, about 10 updates/s |
| `r` / `a` | Clear and arm a new burst |
| `t` | Toggle rising/falling edge |
| `c0` … `c7` | Choose trigger channel |
| `e` | Toggle edge-triggered/automatic capture |
| `n` / `p` | Browse a complete capture |
| `d` | Export a complete capture as CSV: index, relative µs, hexadecimal state |

Send configuration commands first, then `r`. With edge triggering disabled, capture completes automatically after 512 samples. With no suitable edge, arming times out after one second. An edge too near the deadline also times out if its post-trigger samples cannot finish. Timing overruns discard the buffer and produce an explicit error.

During a burst, interrupts are disabled to avoid Timer0/UART jitter; serial input may be lost, buttons are not serviced, and Arduino `millis()`/`micros()` do not track elapsed capture time. Do not send commands until completion/timeout. Timer1 belongs exclusively to capture, so Servo and Timer1 PWM cannot be added concurrently. This is digital state capture and sample browsing, not an analog oscilloscope or a graphical waveform renderer.

## Mode 3: external GPIO peripheral

I expose the peripheral Uno as an I2C target at **0x20** on A4/A5. Its separate D8/D9 bus controls two PCF8574A devices. Wire callbacks access cached registers only; all downstream transfers occur in `loop()`.

| Connection | Wiring |
|---|---|
| Host SDA/SCL | Host A4/A5 → peripheral A4/A5 |
| Expander SDA/SCL | Peripheral D8/D9 → both PCF SDA/SCL |
| Bank 0 address straps | A2=GND, A1=GND, A0=GND → 0x38 |
| Bank 1 address straps | A2=GND, A1=GND, A0=VCC → 0x39 |
| Optional expander interrupt | Both PCF INT outputs → peripheral D3 with pull-up |
| Host interrupt | Peripheral D2 → host D2 with pull-up |
| Reference and power | Common GND; regulated 5 V for Unos/PCFs; decouple each PCF |

Provide external SDA/SCL pull-ups on **each separate bus**. Keep the two buses electrically separate. Do not connect independently powered 5 V outputs together; share grounds and establish a suitable power arrangement.

The host defaults to **10 kHz** so the I2C analyzer can observe it. It configures the AVR TWI prescaler explicitly: `Wire.setClock(10000)` alone cannot represent 10 kHz at 16 MHz with prescaler 1. Set `HOST_I2C_CLOCK_HZ=100000UL` for normal 100 kHz operation without this software sniffer. The expander software bus always remains below 100 kHz.

Register addresses:

| Address | Register | Access and reset |
|---|---|---|
| 0x00 / 0x01 | GPIO0 / GPIO1 output latch | R/W, 0x00 |
| 0x02 / 0x03 | GPIO0 / GPIO1 input snapshot | Read-only; last successful sample |
| 0x04 / 0x05 | Direction0 / Direction1 | R/W, 0xFF; **1=input/released, 0=output** |
| 0x06 | Status | Read-only; bit 0 enabled, bit 1 IRQ asserted, bit 2 downstream bus error |
| 0x07 | Control | R/W, 0x01; bit 0 enables outputs/input-change detection |
| 0x08 | Interrupt status | Bit 0 bank 0 changed, bit 1 bank 1 changed; **write one to clear** |
| 0x09 | Interrupt enable | R/W, 0x00; bits 0/1 enable host IRQ for each bank |
| 0x0A | Device ID | Read-only, 0x42 |
| 0x0B | Debounce interval | R/W, 20 ms reset; 0 bypasses debounce, maximum 255 ms |
| 0x0C / 0x0D | Raw GPIO0 / GPIO1 | Read-only, latest successful unfiltered sample |
| 0x0E / 0x0F | Counter clear masks | Write 1 for each pin counter to clear; reads zero |
| 0x10–0x2F | Sixteen event counters | Read-only, little-endian 16-bit, bank 0 P0 first through bank 1 P7 |

Writing control 0 releases all PCF outputs at the next service pass and clears pending interrupts. For enabled ports, the written PCF byte is `output | direction`: an input must be written high. A PCF high output is weak/quasi-bidirectional, not a general-purpose push-pull high driver.

Write `[register, value...]` to update consecutive registers. Write `[register]` to select a read address, then request **1–32 bytes**; repeated START and STOP-separated reads both work. I return a coherent snapshot of consecutive registers, including complete two-byte counters. The selected read address stays unchanged after a read because Wire does not report how many offered bytes the host consumed. Select the address before every new read; writes still advance through consecutive addresses. Address arithmetic wraps at 256, reserved addresses read zero, and read-only registers ignore writes.

Writes are acknowledged when cached; physical outputs apply asynchronously. Inputs are refreshed after at most a 5 ms scheduling interval **plus transfer time and host interrupt service time** under normal load. Polling is accelerated when the optional PCF INT line is low. Read status before trusting cached inputs: NACK/timeouts set bit 2 and preserve the last good value; reconnection retries the output configuration. I debounce each input independently: a changed level must remain the observed candidate for the configured interval before the input snapshot, interrupt status, and counter update. Each accepted rising or falling transition increments that pin's counter, saturating at 65535. Counter clear and interrupt acknowledgment are independent. Outputs track raw levels without generating input events. Startup, reconnect, disable/re-enable, direction changes, and debounce changes establish fresh baselines. The raw registers expose unfiltered samples. Pulses that change back between samples can still be missed.

Accepted, debounced changes on input-configured bits latch the bank status even when that bank's IRQ is masked. Enabling a bank with pending status asserts the active-low, open-drain host interrupt; clear status to release it. New inputs after startup, reconnection, or direction changes establish a baseline rather than a spurious edge. The host example demonstrates lower-nibble outputs, upper-nibble switches on bank 0, bank 1 inputs, device ID checks, status checks, and interrupt acknowledgment.

## Firmware extensions

I am extending the working hardware configuration through tested firmware stages. Burst reads, configurable switch debounce, and per-pin event counters are now implemented. I record build and regression results for these additions in [validation](docs/validation.md); my earlier hardware confirmation applies to the configuration before these extensions.

## Verification

Physical operation is working as expected on my hardware. I use these checks to catch firmware regressions:

```sh
python3 scripts/test.py
python3 scripts/verify_builds.py
```

The first command exercises production decoding/capture/register code with mocked I/O. The second compiles all four sketches, both display heights, and both host rates; checks a 512-byte minimum static SRAM reserve; and checks the linked logic-capture instruction budget. Use `--libraries PATH` for a U8g2 library outside the Arduino sketchbook. See my [validation results and repeatable bench procedure](docs/validation.md) for the recorded results, environment, and scope of each check.
