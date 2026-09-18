#include <cassert>
#include <cstdio>
#include "capture_session.h"
#include "sampler.h"
#include "display.h"
// Include the actual command parser; hardware sampling/display are mocked here.
#include "../logic-analyzer/logic-analyzer.ino"
void initSampler() {}
uint8_t sampleChannels() { return 0; }
CaptureResult captureBurst(CaptureSession&, const TriggerConfig&) { return CAPTURE_TIMEOUT; }
bool initDisplay() { return true; }
void showLiveState(uint8_t, uint32_t) {}
void showArmedStatus(uint8_t, uint16_t, uint32_t, const TriggerConfig&) {}
void showCaptureSummary(uint16_t, uint16_t, uint16_t, const TriggerConfig&) {}
void showSampleDetail(uint16_t, uint16_t, const Sample&, uint16_t) {}
void showCaptureError(bool) {}
void showWaveform(const CaptureBuffer&, uint16_t, uint8_t, uint8_t, uint16_t) {}
void showPulse(const PulseMeasurement&, uint8_t) {}
void showAnalog(const uint8_t*, uint16_t) {}
uint16_t captureAnalog(uint8_t*, uint16_t) { return 0; }
int main() {
    for (uint8_t ch = 0; ch < 8; ++ch) {
        const uint8_t mask = 1U << ch;
        for (bool rising : {false, true}) {
            CaptureSession capture;
            TriggerConfig cfg = {ch, rising, true};
            assert(triggerDetected(rising ? mask : 0, rising ? 0 : mask, cfg));
            capture.begin(rising ? 0 : mask, cfg);
            const uint32_t triggerAt = 65530; // Capture crosses 16-bit tick wrap.
            for (uint32_t i = 0; i <= triggerAt + POST_TRIGGER_SAMPLES - 1; ++i) {
                const uint8_t state = (i >= triggerAt) == rising ? mask : 0;
                const bool done = capture.add(state);
                assert(done == (i == triggerAt + POST_TRIGGER_SAMPLES - 1));
            }
            assert(capture.buffer.size() == 512);
            assert(capture.buffer.findFirstTickAtOrAfter(capture.triggerTick) == 100);
            Sample first, trg, last;
            assert(capture.buffer.get(0, first) && capture.buffer.get(100, trg) && capture.buffer.get(511, last));
            assert(int16_t(first.tick - capture.triggerTick) == -100);
            assert(int16_t(last.tick - capture.triggerTick) == 411);
            assert(trg.state == (rising ? mask : 0));
            assert(capture.add(0)); assert(capture.buffer.get(511, trg) && trg.tick == last.tick);
            capture.begin(0, {ch, true, false});
            for (unsigned i = 0; i < 512; ++i) assert(capture.add(0) == (i == 511));
            assert(capture.triggerTick == 100); // Automatic capture works with trigger disabled.
        }
    }
    CaptureSession early; early.begin(0, {0, true, true});
    assert(!early.add(1)); // Edge before prehistory is filled is ignored.
    for (int i = 0; i < 1000; ++i) assert(!early.add(1));
    assert(!early.add(0)); assert(!early.add(1));
    for (int i = 0; i < 411; ++i) assert(early.add(1) == (i == 410));
    TriggerConfig invalid = {8, true, true}; assert(!triggerDetected(255, 0, invalid));
    handleCommand('c'); assert(awaitingChannel);
    loop(); assert(awaitingChannel); // A lone c must not block.
    handleCommand('7'); assert(!awaitingChannel && triggerConfig.channel == 7);
    handleCommand('c'); handleCommand('l'); assert(live && !awaitingChannel);
    CaptureBuffer pulses;
    for (uint16_t i = 0; i < 10; ++i) pulses.push({uint16_t(65530 + i), uint8_t(i >= 2 && i < 7)});
    PulseMeasurement pulse;
    assert(measurePulse(pulses, 0, 4, pulse));
    assert(pulse.high && pulse.first == 2 && pulse.end == 7 && pulse.widthUs == 50);
    assert(!pulse.leftClipped && !pulse.rightClipped);
    assert(measurePulse(pulses, 0, 0, pulse) && pulse.leftClipped && pulse.widthUs == 10);
    assert(measurePulse(pulses, 0, 9, pulse) && pulse.rightClipped && pulse.widthUs == 20);
    assert(!measurePulse(pulses, 8, 0, pulse) && !measurePulse(pulses, 0, 10, pulse));
    assert(waveformColumn(pulses, 0, 0, 1) == 0x40);
    assert(waveformColumn(pulses, 0, 2, 1) == 0x7E);
    assert(waveformColumn(pulses, 0, 3, 1) == 0x02);
    assert(waveformColumn(pulses, 0, 0, 8) == 0x7E); // Preserve transitions when zoomed out.
    assert(waveformColumn(pulses, 0, 10, 1) == 0);
    complete = true; session.begin(0, {0, true, false});
    for (unsigned i = 0; i < 512; ++i) session.add(0);
    browseIndex = 0; handleCommand('w'); assert(waveform);
    handleCommand('n'); assert(browseIndex == 120);
    handleCommand('+'); assert(samplesPerPixel == 2);
    handleCommand('m'); assert(!waveform);
    const uint8_t analogBytes[] = {0, 0, 255, 3, 0, 2, 0, 2};
    assert(analogValue(analogBytes, 0) == 0 && analogValue(analogBytes, 1) == 1023);
    assert(analogValue(analogBytes, 2) == 512);
    assert(analogWaveformColumn(analogBytes, 4, 0, 0, 24) == 255);
    assert(analogWaveformColumn(analogBytes, 4, 0, 2, 24) == 255);
    assert(analogWaveformColumn(analogBytes, 4, 2, 0, 24) == 0);
    handleCommand('o'); assert(!complete && !live && !analogCount && session.buffer.size() == 0);
    puts("PASS logic: all channels/edges, exact pre/post counts, timestamp wrap, auto capture, split commands");
}
