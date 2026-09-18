#pragma once
#include <Arduino.h>
static const uint16_t ANALOG_INTERVAL_US = 104; // 13 ADC cycles at 16 MHz / 128.
uint16_t captureAnalog(uint8_t* bytes, uint16_t capacity);
inline uint16_t analogValue(const uint8_t* bytes, uint16_t index) {
    return uint16_t(bytes[index * 2]) | (uint16_t(bytes[index * 2 + 1]) << 8);
}
uint8_t analogWaveformColumn(const uint8_t* bytes, uint16_t count, uint8_t x, uint8_t row, uint8_t height);
