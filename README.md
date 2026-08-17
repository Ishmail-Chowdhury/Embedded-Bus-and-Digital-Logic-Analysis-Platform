# Embedded-Bus-and-Digital-Logic-Analysis-Platform

Mode 1:
I2C Analyzer - Decode transactions

Mode 2:
Logic Analyzer - Capture digital waveforms

Mode 3:
External GPIO Peripheral - Emulates a programmable digital subsystem

## I2CAnalyzer Sketch

The Arduino-ready sketch is in `I2CAnalyzer/` with entry point `I2CAnalyzer.ino`.

### Required Libraries

- Adafruit GFX Library
- Adafruit SSD1306

Install both from Arduino IDE: `Sketch -> Include Library -> Manage Libraries...`

### Elegoo Uno R3 Wiring

- OLED VCC -> 5V
- OLED GND -> GND
- OLED SDA -> A4
- OLED SCL -> A5
- Next button -> D2 (use `INPUT_PULLUP`, button to GND)
- Prev button -> D3 (use `INPUT_PULLUP`, button to GND)

### Upload Steps

1. Open `I2CAnalyzer/I2CAnalyzer.ino` in Arduino IDE.
2. Select board: `Arduino Uno`.
3. Select correct serial port.
4. Upload.
5. Open Serial Monitor at `115200` baud for debug lines.

### Notes

- The analyzer uses A4/A5 bus sampling.
- OLED control traffic is filtered by address `0x3C` before packets are pushed into history so the ring buffer reflects target-device traffic instead of display self-traffic.
- Ring buffer stores the latest 32 decoded packets and browsing is done with the two buttons.
