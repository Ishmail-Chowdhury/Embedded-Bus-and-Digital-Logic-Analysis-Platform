#include "capture_buffer.h"
bool CaptureBuffer::get(uint16_t index, Sample& out) const {
    if (index >= count) return false;
    const uint16_t oldest = (head + CAPTURE_BUFFER_SIZE - count) & (CAPTURE_BUFFER_SIZE - 1);
    out.state = buffer[(oldest + index) & (CAPTURE_BUFFER_SIZE - 1)];
    out.tick = static_cast<uint16_t>(newestTick - count + 1 + index);
    return true;
}
int16_t CaptureBuffer::findFirstTickAtOrAfter(uint16_t tick) const {
    Sample sample;
    for (uint16_t i = 0; i < count; ++i) {
        get(i, sample);
        if (static_cast<int16_t>(sample.tick - tick) >= 0) return static_cast<int16_t>(i);
    }
    return -1;
}
