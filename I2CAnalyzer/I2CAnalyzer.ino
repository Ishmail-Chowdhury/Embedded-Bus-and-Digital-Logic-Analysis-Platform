#include <Arduino.h>
#include "config.h"
#include "bus_sampler.h"
#include "capture.h"
#include "ring_buffer.h"
#include "display.h"
#include "bus_filter.h"
#include "fast_capture.h"
static BusFilter filter;
static bool filtering = false;
static uint16_t rawSamples = 0;

static void acceptState(const BusState& state) {
    BusState accepted;
    if (filter.push(state, accepted)) decodeBusState(accepted);
}
static void settleFilter() {
    uint32_t now; BusState accepted;
    if (busCaptureIdleTime(now) && filter.settle(now, accepted)) decodeBusState(accepted);
}
static bool capturing = false;
static int selectedIndex = 0;
static uint8_t lastButtons = 3;
static uint32_t lastButtonAt = 0;

static void arm() {
    stopBusCapture();
    rawSamples = 0;
    showCaptureStatus(true, false);
    Serial.println(F("Capture armed; s or either button pauses. <=10 kHz target."));
    Serial.flush();
    initRingBuffer(); resetDecoder(); selectedIndex = 0;
    // Require idle before arming, so attaching mid-byte cannot invent a packet.
    const BusState state = readBus();
    if (!state.sda || !state.scl) {
        capturing = false;
        showCaptureStatus(false, false);
        Serial.println(F("Bus busy/stuck; stop the source and send r again"));
        return;
    }
    filter.reset(filtering ? GLITCH_FILTER_US : 0);
    startBusCapture(); capturing = true;
}
static void fastCapture() {
    stopBusCapture(); capturing = false; rawSamples = 0;
    resetDecoder(); selectedIndex = 0;
    showFastStatus(false);
    Serial.println(F("Experimental 1MS/s burst: waiting <=1s for START. No commands until done."));
    Serial.flush();
    rawSamples = captureFastBus(packetScratch(), FAST_CAPTURE_BYTES);
    while (Serial.available()) Serial.read();
    if (rawSamples) {
        showFastStatus(true);
        Serial.println(F("1024 samples; d=raw CSV x=decode. Times from first sample; first START approximated as 0."));
    } else {
        showCaptureStatus(false, false);
        Serial.println(F("Fast capture empty: bus busy at arm or START timed out; f retries."));
    }
}
static void exportFast() {
    Serial.println(F("sample,relative_us,sda,scl"));
    for (uint16_t i = 0; i < rawSamples; ++i) {
        const BusState state = fastBusState(rawBusCapture(), i);
        Serial.print(i); Serial.print(','); Serial.print(state.atUs); Serial.print(',');
        Serial.print(state.sda ? 1 : 0); Serial.print(','); Serial.println(state.scl ? 1 : 0);
    }
}
static void decodeFast() {
    Serial.println(F("Offline decode; first START_us=0 approximate; final unfinished phase flagged 0x10."));
    // Print directly: writing packet history would overwrite the raw capture.
    setPacketSink(printPacket);
    decodeFastBus(rawBusCapture(), rawSamples);
    setPacketSink(nullptr);
}
static void pauseCapture(bool overflowed) {
    stopBusCapture(); capturing = false;
    // Drain complete segments. Discard any transaction that was cut short.
    BusState state;
    if (!overflowed) while (nextBusState(state)) acceptState(state);
    if (!overflowed) settleFilter();
    resetDecoder(); selectedIndex = packetCount() ? packetCount() - 1 : 0;
    showCaptureStatus(false, overflowed);
    Serial.println(overflowed ? F("EDGE QUEUE OVERFLOW: capture invalid after edge loss") : F("Paused; n/p browse, r clears and rearms"));
}
static void browse(char cmd) {
    const int count = packetCount();
    if (cmd == 'n' && selectedIndex + 1 < count) ++selectedIndex;
    if (cmd == 'p' && selectedIndex > 0) --selectedIndex;
    Packet packet = {};
    getPacket(selectedIndex, packet);
    updateDisplay(count, packet, selectedIndex);
}
void setup() {
    Serial.begin(115200); initBusSampler(); initDisplay();
    pinMode(BUTTON_NEXT, INPUT_PULLUP); pinMode(BUTTON_PREV, INPUT_PULLUP);
    initRingBuffer(); resetDecoder(); showCaptureStatus(false, false);
    Serial.println(F("I2C: r=slow s=pause n/p=browse g=filter f=fast d=raw x=decode. OLED D8/D9."));
}
void loop() {
    if (capturing) {
        BusState state;
        // Bounded drain also lets the user pause a continuous bus.
        for (uint8_t i = 0; i < EDGE_QUEUE_SIZE && nextBusState(state); ++i) acceptState(state);
        settleFilter();
        if (busCaptureOverflowed()) pauseCapture(true);
    }
    if (Serial.available()) {
        const char cmd = Serial.read();
        if (cmd == 's' && capturing) pauseCapture(false);
        else if (cmd == 'r') { capturing = false; arm(); }
        else if (!capturing && cmd == 'f') fastCapture();
        else if (!capturing && rawSamples && cmd == 'd') exportFast();
        else if (!capturing && rawSamples && cmd == 'x') decodeFast();
        else if (!capturing && cmd == 'g') {
            filtering = !filtering;
            Serial.println(filtering ? F("Filter 8 us; applies on next arm") : F("Filter off"));
        }
        else if (!capturing && !rawSamples && (cmd == 'n' || cmd == 'p')) browse(cmd);
    }
    const uint8_t buttons = (digitalRead(BUTTON_NEXT) != LOW) | ((digitalRead(BUTTON_PREV) != LOW) << 1);
    const uint32_t now = millis();
    if (buttons != lastButtons && now - lastButtonAt >= 30) {
        const uint8_t pressed = lastButtons & ~buttons;
        lastButtons = buttons; lastButtonAt = now;
        if (pressed) {
            if (capturing) pauseCapture(false);
            else if (!rawSamples) browse((pressed & 1) ? 'n' : 'p');
        }
    }
}
