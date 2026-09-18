#include "analog_capture.h"
uint8_t analogWaveformColumn(const uint8_t* bytes, uint16_t count, uint8_t x, uint8_t row, uint8_t height) {
    const uint16_t index = uint16_t(x) * 2;
    if (!bytes || index >= count || !height || row >= height / 8) return 0;
    const uint16_t last = index + 1 < count ? index + 1 : index;
    uint8_t low = height - 1, high = 0;
    // Include the preceding sample to connect adjacent columns and retain extrema.
    for (uint16_t i = index ? index - 1 : index; i <= last; ++i) {
        uint16_t value = analogValue(bytes, i);
        if (value > 1023) value = 1023;
        const uint8_t y = height - 1 - (uint32_t(value) * (height - 1) / 1023);
        if (y < low) low = y;
        if (y > high) high = y;
    }
    uint8_t pixels = 0;
    for (uint8_t bit = 0; bit < 8; ++bit) {
        const uint8_t y = row * 8 + bit;
        if (y >= low && y <= high) pixels |= (1 << bit);
    }
    return pixels;
}
