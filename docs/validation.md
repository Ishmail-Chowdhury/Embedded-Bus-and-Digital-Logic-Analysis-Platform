# Validation record and hardware acceptance procedure

## Scope and result

The firmware defects found in the original repository were corrected and the checks below were run locally. **This is software/build validation, not a hardware pass.** `arduino-cli board list` found only generic Bluetooth/debug serial ports, with no recognized connected Uno. No firmware was uploaded, and no electrical or timing measurements were taken.

The exact purchased parts list, OLED model, circuit diagram, load currents, and any requirements beyond the original README were unavailable. Confirm these against [parts and constraints](parts-and-constraints.md) before final acceptance.

## Defects addressed

| Original defect | Corrected behavior |
|---|---|
| 5 ms delay between I2C bus samples; blocking button/display work | Pin-change edge queue; decoding during capture; display/browser only while paused |
| OLED actively sharing the observed bus | Separate D8/D9 display bus; external 0x3C traffic remains observable |
| Repeated START reset discarded the preceding transaction phase | Preserve each address phase and flag repeated START |
| Ninth clock ignored and long payloads silently truncated | Record NACK and its first position; flag truncation and incomplete-byte termination |
| Display/Serial stalls counted as 10 µs logic samples | Exclusive Timer1 burst capture; reject timing overruns |
| 100,000 Hz narrowed to 16 bits | 32-bit display rate; live UI rate distinguished from burst sample rate |
| Post-trigger count off by one; ordinary comparisons broke at tick rollover | 100 pre + 412 including trigger; wrap-aware tick lookup |
| Trigger disabled never finished a capture; lone `c` blocked indefinitely | Automatic capture and incremental serial parser |
| Old buffer mixed samples from different arm periods | Fresh contiguous history for every arm; explicit `a`/`r` alias |
| Timestamp-per-sample storage consumed 1536 bytes before other globals | 512 state bytes with reconstructed ticks |
| PCF8574A used non-A addresses, including the peripheral's address | Downstream 0x38/0x39; host-facing target 0x20 on separate bus |
| Direction masks never affected port writes; startup drove inputs low | Released inputs via `output OR direction`; all-input reset |
| Nested Wire transfers and Serial inside Wire callbacks | Cached ISR-safe register callbacks; downstream I/O in foreground |
| Pointer-only writes ignored; reads usually returned register zero | Persistent register pointer with repeated START support |
| ID/input/status writable; control disable ineffective | Enforced access policy and defined enable/disable behavior |
| Interrupt enable, event generation and acknowledgment incomplete | Input-change bank latches, masks, W1C and open-drain host signal |
| Failed expander reads appeared as valid zero data | Preserve cached data, report STATUS_ERROR, retry after reconnection |
| Low-rate host clock and reset recovery not accounted for | Explicit 10 kHz TWI prescaler; restore it after Wire timeout resets |

## Reproducible checks

Tools used: Arduino CLI 1.5.1, Arduino AVR Boards 1.8.8, AVR GCC 7.3.0, U8g2 2.36.18, Python 3.12, Apple Clang 17. U8g2 was downloaded into a temporary library directory; no global Arduino libraries or system compiler settings were changed.

```sh
# Native regression suites, warnings as errors, UndefinedBehaviorSanitizer:
python3 scripts/test.py

# Default sketches, alternate display heights, alternate host clock,
# static memory reserve, and linked AVR instruction budget:
python3 scripts/verify_builds.py --libraries /path/to/additional/libraries
```

Omit `--libraries` when U8g2 is installed in the Arduino sketchbook. The build script accepts `--arduino-cli` and `--avr-objdump` for tools outside normal locations. On macOS it also locates the Arduino IDE's bundled CLI. The native test script uses the selected macOS SDK's C++ headers when necessary, with no persistent system changes; `CXX` and `CXXFLAGS` override that behavior.

Five native suites passed with UndefinedBehaviorSanitizer:

- I2C: edge classification, attach-mid-transaction, ACK/NACK, repeated START read, address NACK, external 0x3C, oversized payload, unsupported 10-bit header, partial byte, 32-packet history rollover and invalid indexes.
- Logic: every channel and both edges, trigger-window counts, 16-bit tick rollover, disabled-trigger automatic capture, early-edge rejection, capture completion immutability and split serial commands.
- Peripheral: pointer-only writes, sequential writes, cached callbacks, physical direction masks, read-only registers, bank IRQ status/masks/W1C, disable/re-enable, input-vs-output changes, missing device/recovery and a host write injected during an input read.
- Host: 10 kHz prescaler arithmetic, repeated START, address/data failures, short reads, and restoring the prescaler after a Wire timeout.
- Downstream bus: ACK/NACK, read-byte assembly, preservation of data on error, bounded stuck-clock timeout, and startup recovery.

`SANITIZERS=address,undefined python3 scripts/test.py` optionally enables AddressSanitizer on a compatible host. An ASan-instrumented executable terminated with SIGILL on this machine; no ASan pass is claimed. The default UBSan suites passed. Mocked I/O does not emulate AVR interrupt arbitration, analog waveforms or real devices.

## Uno build results

All seven configurations compile. Compiler warnings seen were unused parameters in the installed Arduino core's `new.cpp`; no project-source warning was reported. The table records static allocation, not a measured stack high-water mark.

| Sketch / configuration | Flash bytes | Static SRAM bytes | Remaining SRAM bytes |
|---|---:|---:|---:|
| I2C analyzer, 128×32 | 11,268 | 1,412 | 636 |
| I2C analyzer, 128×64 | 11,280 | 1,412 | 636 |
| Logic analyzer, 128×64 | 11,354 | 1,295 | 753 |
| Logic analyzer, 128×32 | 11,342 | 1,295 | 753 |
| Peripheral | 4,002 | 227 | 1,821 |
| Host, 10 kHz | 5,100 | 404 | 1,644 |
| Host, 100 kHz | 5,096 | 404 | 1,644 |

The verification script requires ≥512 bytes beyond static SRAM use. That is a screening margin, not proof of worst-case runtime stack use. The analyzer display uses U8x8 without a framebuffer; firmware does not use Arduino `String` or dynamic capture allocation.

For both OLED configurations, the linked AVR capture loop's conservative control-flow bound was **135/160 CPU cycles**, or **8.4375 µs of work within each 10 µs interval**. The check includes both sides of conditional branches and polling phase allowance. It excludes paths that terminate capture. Timer configuration is `/8`, CTC, `OCR1A=19`. Runtime checks reject a late timer phase or work extending into the next deadline.

This establishes CPU-budget feasibility for these exact builds. It does not measure oscillator error, metastability, pin loading or physical sample timing. Port D and port C are separate reads (one instruction apart in this build), so the eight channels are not perfectly simultaneous.

## Bench acceptance checklist — not yet executed

Record the exact board/IC/module markings, firmware revision, supply voltage, instruments, pull-up values and pass/fail measurements for each item. Use a reference oscilloscope or independently qualified analyzer; do not use this firmware to certify its own timing.

### 1. Wiring, voltage and power

1. Confirm both ICs are PCF8574A and straps yield 0x38/0x39. Check package pin numbers from the exact package datasheet.
2. Confirm OLED resolution, address, supply rating and signal tolerance. Provide translation where necessary.
3. Verify common grounds, independent host/expander/display buses, decoupling and pull-ups. Measure effective pull-up resistance including breakouts. Verify that independently powered 5 V outputs are not tied together.
4. Before attaching observation inputs, measure their voltage range. Measure SDA/SCL idle highs, low levels and rise times with all probes connected. Compare with the component and I2C limits.

### 2. Peripheral and host

1. Upload the peripheral and host to their identified boards. The host should print `Peripheral ID 0x42 OK`, with no downstream bus error once both expanders respond.
2. Verify ID read on the reference analyzer: `START 0x20/W ACK 0x0A ACK RESTART 0x20/R ACK 0x42 NACK STOP`.
3. Observe the default 10 kHz host clock and the downstream clock below 100 kHz. Repeat host verification with `HOST_I2C_CLOCK_HZ=100000UL` when the software sniffer is disconnected or paused.
4. Check bank 0 P0–P3 against output latch 0x05: P0/P2 released/high, P1/P3 low. P4–P7 remain inputs. The expected unloaded input byte is approximately 0xF5, subject to the actual connected circuit and valid thresholds.
5. Ground and release each input switch. Verify input snapshots, correct bank bits in 0x08, active-low D2 IRQ and release after writing the pending mask to 0x08. Verify mask/unmask behavior and that output-only changes do not create input interrupts. Observe bounce rather than assuming one press equals one event.
6. Write control 0 and confirm outputs release on the next service pass and IRQ deasserts. Re-enable, verify the configured latch returns, and confirm no artificial startup interrupt. Attempts to write ID/input/status must have no effect.
7. Disconnect each expander and hold downstream SDA or SCL low in a controlled test. Confirm upstream register reads remain responsive, status bit 2 asserts and old input data is marked stale. Reconnect/release and confirm retries recover without MCU reset. Verify host timeout recovery preserves 10 kHz after the upstream bus is released.
8. Measure write-to-pin and input-to-IRQ latency under expected host traffic. There is no claimed hard bound under arbitrary upstream traffic, no pulse counter, and no switch debounce.

### 3. I2C analyzer

1. With the source idle, arm using `r`, then generate the device-ID sequence above at 10 kHz. Pause with `s` and browse. Expect two phases: 0x20 W containing 0x0A with repeated-START flag, then 0x20 R containing 0x42 with final-data NACK at position 1.
2. Compare a long known stream against a reference analyzer. Verify no phantom OLED traffic, no missing address phases, latest-32 retention and payload truncation flags for >16-byte transfers.
3. Test both buttons, held buttons, address NACK, clock stretching, incomplete transactions, START/repeated START/STOP timing and attaching with a busy bus. Rendering must occur only when paused.
4. Qualify worst-case edge spacing and interrupt latency at the intended rate. If packets differ or overflow occurs, the rate is not accepted. Do not infer 100/400 kHz support from a successful slow-bus test.

### 4. Logic analyzer

1. Apply a measured slow square wave, e.g. 1 kHz, to each channel in turn; keep other channels at defined levels. Select the corresponding `cN` and edge, then arm. Capture should complete with the trigger at index 100 (zero-based).
2. Export using `d`. Check 512 rows, timestamps −1000 through +4110 µs in 10 µs increments, and channel mapping/state changes against the reference waveform. Confirm rising and falling triggers on all channels.
3. Disable edge triggering using `e` and arm; it should complete automatically in approximately 5.12 ms. Hold the selected signal constant with edge triggering enabled; it should time out after approximately one second and report no valid capture.
4. Verify one-second timeout also handles an edge arriving too late to collect all post-trigger samples. Confirm `c` alone leaves the UI responsive outside a burst and repeated arms never mix history.
5. Measure sampling jitter, channel-to-channel skew and input-loading effects. Re-run the CPU-budget check after changes to compiler, sampling code or configuration. Live OLED refresh is about 10 Hz and is not the acquisition rate.

### 5. Final acceptance

Run the expected continuous workload, exercise error recovery repeatedly, and measure stack high-water usage. Compare measured behavior with the user's actual speed, latency, voltage and load requirements. Hardware acceptance remains open until those requirements and measurements are recorded.
