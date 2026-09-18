#pragma once
#include <Arduino.h>
#include "config.h"
struct Sample { uint16_t tick; uint8_t state; };
class CaptureBuffer {
public:
    CaptureBuffer() : head(0), count(0), newestTick(0) {}
    void clear() { head = count = newestTick = 0; }
    // Samples in a capture must have consecutive ticks (modulo 65536).
    // Store only state: timestamps are reconstructed without 1024 redundant SRAM bytes.
    void push(const Sample& sample) {
        buffer[head] = sample.state;
        head = (head + 1) & (CAPTURE_BUFFER_SIZE - 1);
        if (count < CAPTURE_BUFFER_SIZE) ++count;
        newestTick = sample.tick;
    }
    // Mutually exclusive analog mode reuses the state buffer; invalidates digital history.
    uint8_t* scratchData() { clear(); return buffer; }
    const uint8_t* rawData() const { return buffer; }
    bool full() const { return count == CAPTURE_BUFFER_SIZE; }
    uint16_t size() const { return count; }
    uint16_t capacity() const { return CAPTURE_BUFFER_SIZE; }
    bool get(uint16_t index, Sample& out) const;
    int16_t findFirstTickAtOrAfter(uint16_t tick) const;
private:
    uint8_t buffer[CAPTURE_BUFFER_SIZE];
    uint16_t head, count, newestTick;
};
