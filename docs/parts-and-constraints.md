# Parts, datasheets, and design constraints

I use this document as the functional parts reference for my platform and the datasheet basis for its operating limits. I have confirmed that physical operation is working as expected on my hardware. The table identifies component families, quantities by configuration, and compatibility requirements; exact vendor order codes, the OLED module model, and a measured electrical characterization are not recorded here.

## Parts and configuration

| Part | Quantity / purpose | Configuration and compatibility |
|---|---|---|
| Elegoo/Arduino Uno R3, 16 MHz ATmega328P | One per analyzer; two for host/peripheral | Three boards for peripheral + one analyzer; four for both analyzers simultaneously |
| SSD1306 I2C OLED module | One per analyzer in simultaneous use | Firmware supports 128×32 and 128×64; defaults differ by analyzer. Match the height, 0x3C/0x3D address, and voltage ratings to the selected module |
| PCF8574A | Two on peripheral downstream bus | Confirm **A** suffix. Non-A PCF8574 uses a different address range; defaults will not work unchanged |
| Momentary buttons | Two for I2C browser; optional switches on PCF inputs | Browser buttons to ground, internal pull-ups; PCF switches only on released/input pins |
| External SDA/SCL pull-up resistors | Two per electrically separate I2C bus | Include any module pull-ups when calculating effective parallel resistance |
| Interrupt pull-ups | One on each used IRQ net | Peripheral D2 → host D2; optional shared PCF INT → peripheral D3 |
| Bypass capacitors | 100 nF close to each bare PCF supply pair | Check what each breakout already includes; add local bulk decoupling as supply wiring requires |
| LEDs and resistors, optional | Demonstrate output sinks | Connect supply → resistor → LED → PCF pin; calculate current for the actual LED |
| Breadboard, jumpers, regulated supplies, USB cables | As needed | Common signal ground, short bus wiring, appropriate supply arrangement |
| Level translation / input protection | As required by the actual targets/OLED | Uno pins are not universal protected probes; do not infer module tolerance from the controller name |

## Datasheet references

I used these references to check pin assignments, memory use, bus behavior, and electrical limits.

| Source | Sections used | Design consequence |
|---|---|---|
| [Microchip ATmega328P datasheet, 7810D](https://ww1.microchip.com/downloads/en/devicedoc/atmel-7810-automotive-microcontrollers-atmega328p_datasheet.pdf) | SRAM, GPIO, pin-change interrupts, Timer1, TWI, DC characteristics | 2 KiB SRAM; 16 MHz CPU budget; hardware scheduling; explicit bus prescaler; logic-level limits. This is the manufacturer's automotive edition; confirm the actual chip grade separately |
| [Arduino Uno R3 datasheet](https://docs.arduino.cc/resources/datasheets/A000066-datasheet.pdf) | Board pinout, memory, power | Uno pin assignments and resource assumptions; verify the Elegoo board implementation against its own documentation |
| [TI PCF8574A datasheet, SCPS069H](https://www.ti.com/lit/ds/symlink/pcf8574a.pdf) | 5.3–5.7, 7.1–7.4, 8.3–8.4 | Correct addresses, released inputs, clock/current/voltage limits, decoupling and interrupt behavior |
| [NXP I2C specification, UM10204 Rev. 7](https://cache.nxp.com/docs/en/user-guide/UM10204.pdf) | 3.1, 6, 7.1 | START/STOP/repeated START, ninth-bit ACK, open-drain wiring, pull-up sizing |
| [Solomon Systech SSD1306 datasheet via Adafruit](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf) | I2C interface, supply and DC limits | Controller-level limits do not establish breakout-board 5 V tolerance |
| [Arduino AVR Wire implementation](https://github.com/arduino/ArduinoCore-avr/blob/master/libraries/Wire/src/Wire.cpp) and [TWI ISR](https://github.com/arduino/ArduinoCore-avr/blob/master/libraries/Wire/src/utility/twi.c) | onReceive/onRequest dispatch, transmit/receive, timing | Callback context must not perform nested blocking transfers; cache register values and use an independent downstream bus |
| [U8g2/U8x8 library](https://github.com/olikraus/u8g2) | U8x8 software-I2C backend and display constructors | Text display avoids framebuffer allocation; software-I2C may enable MCU pull-ups, so level compatibility still matters |

## Electrical constraints

I designed the observation inputs for 0–5 V digital signals with a common ground on a 5 V Uno. Ordinary ATmega328P GPIO requires a high of at least 0.6×VCC and a low no higher than 0.3×VCC. A 1.8 V target needs translation. Do not apply negative, RS-232, automotive, or mains-level signals directly. Absolute-maximum voltage/current figures are damage limits, not operating targets. See the [Microchip DC characteristics](https://ww1.microchip.com/downloads/en/devicedoc/atmel-7810-automotive-microcontrollers-atmega328p_datasheet.pdf).

PCF8574A operation allows 2.5–6 V, SCL ≤100 kHz, high time ≥4 µs and low time ≥4.7 µs. At 5 V its guaranteed input-high threshold is 3.5 V, so a bus pulled only to 3.3 V is not guaranteed. Inputs must have their latch bits high. Port highs are weak current-source levels; use sinking for LEDs and choose conservative load currents. Package total current, thermal limits and output-voltage limits also apply; do not interpret a per-pin limit as permission to load all pins equally to it. Address straps must be fixed, and INT is open drain. These constraints come from the [TI datasheet](https://www.ti.com/lit/ds/symlink/pcf8574a.pdf).

The raw SSD1306 logic supply is specified up to 3.3 V. A module may include a regulator and may or may not include logic translation. I base the OLED power and signal connections on the module manufacturer's specifications; an SSD1306 controller name alone does not establish 5 V compatibility. U8x8 software-I2C can enable pull-ups to the Uno's supply; merely powering the display from 3.3 V does not establish compatible signal voltages. Use a module documented for 5 V logic or appropriate bidirectional translation. See the [SSD1306 controller datasheet](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf).

I keep the upstream, downstream and display buses separate, with pull-ups on each bus. For Standard-mode, size using `Rmin = (Vpullup − VOLmax) / IOL` and `Rmax = tr_max / (0.8473 × Cbus)`. At 5 V, VOL=0.4 V and a 3 mA sink, Rmin is about 1.53 kΩ. At 200 pF and a 1 µs rise-time limit, Rmax is about 5.9 kΩ. A 4.7 kΩ pair is a starting value for that example, not a universal value; parallel module resistors, cables, probes and translators change the result. Measure the assembled bus. See [NXP pull-up sizing](https://cache.nxp.com/docs/en/user-guide/UM10204.pdf).

## Functional result and constraint checks

I have confirmed expected physical operation of the platform. I retain the following implementation checks and measurement references so that changes to the firmware, wiring, or components can be evaluated against the same limits. The measurement column identifies what to characterize for a particular setup; it does not record individual instrument readings or fault-injection results.

| Requirement | Implementation and recorded checks | Measurement or repeat-test reference |
|---|---|---|
| Uno SRAM ≤2048 bytes | All variants compiled; static allocation restricted to ≤1536 bytes by build script | Stack high-water usage under the intended workload |
| Nominal 100 kS/s logic capture | Timer1 /8, OCR1A=19; linked CPU paths checked against 160 cycles; overruns fail explicitly | Oscillator accuracy, sampling jitter, asynchronous input behavior, loading |
| 512 samples and pre/post split | Native tests across all channels and both edges, including tick rollover | Known-waveform comparison |
| Contiguous capture timestamps | Capture/display separated; each arm clears history | Reference-analyzer comparison after sampling changes |
| I2C passive observation | A4/A5 inputs; OLED isolated; queue captures port snapshots | Bus rate, START/STOP visibility and minimum edge spacing |
| I2C packet semantics | 7/10-bit addressing, ACK/NACK, repeated START, timestamp/filter wrap, payload and history bounds tested | Known transaction stream comparison |
| PCF input/output behavior | `latch OR direction`; independent bus; ACK/timeouts and retry | Part suffix, address straps, physical levels and load current |
| Register-mapped target | Selected start address; coherent 1–32-byte reads; cached callbacks; read-only registers | Repeated START transactions and clock stretching |
| Input-change interrupt | Per-pin configurable debounce and saturating counters; bank IRQ masks/W1C; released host line | Debounce latency, missed sub-sample pulses and pull-up loading |
| Missing/stuck device | Bounded software-SCL wait and host Wire timeout; stale data marked | Disconnect/reconnect and stuck SDA/SCL regression tests |

I keep the sniffer target at 10 kHz, below the PCF/host 100 kHz capability. Pin-change hardware coalesces edges arriving while interrupts are masked; an overflow flag cannot prove that all physical edges were seen. Slow the host link for this analyzer, or use a separately qualified analyzer for 100/400 kHz traffic. The 100 kS/s logic sampler also does not guarantee recognition of pulses shorter than a sample interval or correct reconstruction of signals close to its sampling rate.
