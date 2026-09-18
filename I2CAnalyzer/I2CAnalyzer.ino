#include <Arduino.h>
#include "config.h"
#include "bus_sampler.h"
#include "capture.h"
#include "ring_buffer.h"
#include "display.h"
static bool capturing = false;
static int selectedIndex = 0;
static uint8_t lastButtons = 3;
static uint32_t lastButtonAt = 0;

static void arm() {
    stopBusCapture();
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
    startBusCapture(); capturing = true;
}
static void pauseCapture(bool overflowed) {
    stopBusCapture(); capturing = false;
    // Drain complete segments. Discard any transaction that was cut short.
    BusState state;
    if (!overflowed) while (nextBusState(state)) decodeBusState(state);
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
    Serial.println(F("I2C sniffer: r=clear/arm, s=pause, n/p=browse. OLED on D8/D9."));
}
void loop() {
    if (capturing) {
        BusState state;
        // Bounded drain also lets the user pause a continuous bus.
        for (uint8_t i = 0; i < EDGE_QUEUE_SIZE && nextBusState(state); ++i) decodeBusState(state);
        if (busCaptureOverflowed()) pauseCapture(true);
    }
    if (Serial.available()) {
        const char cmd = Serial.read();
        if (cmd == 's' && capturing) pauseCapture(false);
        else if (cmd == 'r') { capturing = false; arm(); }
        else if (!capturing && (cmd == 'n' || cmd == 'p')) browse(cmd);
    }
    const uint8_t buttons = (digitalRead(BUTTON_NEXT) != LOW) | ((digitalRead(BUTTON_PREV) != LOW) << 1);
    const uint32_t now = millis();
    if (buttons != lastButtons && now - lastButtonAt >= 30) {
        const uint8_t pressed = lastButtons & ~buttons;
        lastButtons = buttons; lastButtonAt = now;
        if (pressed) {
            if (capturing) pauseCapture(false);
            else browse((pressed & 1) ? 'n' : 'p');
        }
    }
}
