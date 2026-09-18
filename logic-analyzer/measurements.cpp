#include "measurements.h"
bool measurePulse(const CaptureBuffer& capture, uint8_t channel, uint16_t index, PulseMeasurement& out) {
    Sample sample;
    if (channel >= NUM_CHANNELS || !capture.get(index, sample)) return false;
    const uint8_t mask = 1 << channel;
    out.high = sample.state & mask;
    out.first = index; out.end = index + 1;
    while (out.first && capture.get(out.first - 1, sample) && bool(sample.state & mask) == out.high) --out.first;
    while (capture.get(out.end, sample) && bool(sample.state & mask) == out.high) ++out.end;
    out.leftClipped = out.first == 0; out.rightClipped = out.end == capture.size();
    // Clipped runs establish a lower bound only between their observed samples.
    uint16_t intervals = out.end - out.first;
    if (out.leftClipped || out.rightClipped) --intervals;
    out.widthUs = uint32_t(intervals) * SAMPLE_INTERVAL_US;
    return true;
}
uint8_t waveformColumn(const CaptureBuffer& capture, uint8_t channel, uint16_t index, uint8_t samplesPerPixel) {
    Sample sample;
    if (channel >= NUM_CHANNELS || !samplesPerPixel || !capture.get(index, sample)) return 0;
    const uint8_t mask = 1 << channel;
    const bool first = sample.state & mask;
    bool transition = index && capture.get(index - 1, sample) && bool(sample.state & mask) != first;
    for (uint8_t i = 1; i < samplesPerPixel && capture.get(index + i, sample); ++i)
        transition |= bool(sample.state & mask) != first;
    return transition ? 0x7E : (first ? 0x02 : 0x40);
}
