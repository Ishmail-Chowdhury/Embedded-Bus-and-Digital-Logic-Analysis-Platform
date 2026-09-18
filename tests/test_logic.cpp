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
    puts("PASS logic: all channels/edges, exact pre/post counts, timestamp wrap, auto capture, split commands");
}
