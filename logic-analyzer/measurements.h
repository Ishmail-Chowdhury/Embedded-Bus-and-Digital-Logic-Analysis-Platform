#pragma once
#include "capture_buffer.h"
struct PulseMeasurement {
    uint16_t first, end; // end is exclusive; buffer.size() means right-clipped.
    uint32_t widthUs;
    bool high, leftClipped, rightClipped;
};
bool measurePulse(const CaptureBuffer& capture, uint8_t channel, uint16_t index, PulseMeasurement& out);
uint8_t waveformColumn(const CaptureBuffer& capture, uint8_t channel, uint16_t index, uint8_t samplesPerPixel);
