#include <Arduino.h>
#include "config.h"
#include "sampler.h"
#include "trigger.h"
#include "capture_session.h"
#include "display.h"
static CaptureSession session;
static TriggerConfig triggerConfig = {DEFAULT_TRIGGER_CHANNEL, DEFAULT_TRIGGER_RISING, true};
static bool live = true, complete = false, awaitingChannel = false;
static uint16_t browseIndex = 0;
static uint32_t lastDisplayUpdateMs = 0;

static void armCapture() {
    showArmedStatus(sampleChannels(), 0, 100000UL, triggerConfig);
    Serial.println(F("ARMED: up to 1 second; do not send commands during capture"));
    Serial.flush();
    const CaptureResult result = captureBurst(session, triggerConfig);
    // UART cannot service traffic during the burst; discard any pending input.
    while (Serial.available()) Serial.read();
    complete = result == CAPTURE_OK; live = false;
    if (!complete) {
        Serial.println(result == CAPTURE_TIMEOUT ? F("TIMEOUT: no trigger; r to retry") : F("TIMING OVERRUN: capture discarded"));
        showCaptureError(result == CAPTURE_TIMEOUT);
        return;
    }
    browseIndex = static_cast<uint16_t>(session.buffer.findFirstTickAtOrAfter(session.triggerTick));
    showCaptureSummary(session.buffer.size(), browseIndex, session.buffer.size() - browseIndex, triggerConfig);
    Serial.println(F("CAPTURE COMPLETE: 512 samples, 100 before / 412 including trigger"));
}
static void browse(char cmd) {
    if (!complete) return;
    if (cmd == 'n' && browseIndex + 1 < session.buffer.size()) ++browseIndex;
    if (cmd == 'p' && browseIndex > 0) --browseIndex;
    Sample sample;
    if (session.buffer.get(browseIndex, sample)) showSampleDetail(browseIndex, session.buffer.size(), sample, session.triggerTick);
}
static void dumpCapture() {
    if (!complete) return;
    Serial.println(F("index,relative_us,state_hex"));
    Sample sample;
    for (uint16_t i = 0; i < session.buffer.size(); ++i) {
        session.buffer.get(i, sample);
        Serial.print(i); Serial.print(',');
        Serial.print(static_cast<int16_t>(sample.tick - session.triggerTick) * static_cast<int32_t>(SAMPLE_INTERVAL_US));
        Serial.print(','); Serial.println(sample.state, HEX);
    }
}
static void handleCommand(char cmd) {
    if (awaitingChannel) {
        awaitingChannel = false;
        if (cmd >= '0' && cmd <= '7') {
            triggerConfig.channel = cmd - '0';
            Serial.print(F("Trigger CH")); Serial.println(triggerConfig.channel);
            return;
        }
        Serial.println(F("Expected c0..c7"));
    }
    switch (cmd) {
    case 'c': awaitingChannel = true; break; // No blocking wait for a second character.
    case 'a': case 'r': armCapture(); break;
    case 'l': live = true; complete = false; break;
    case 't': triggerConfig.risingEdge = !triggerConfig.risingEdge;
        Serial.println(triggerConfig.risingEdge ? F("Rising edge") : F("Falling edge")); break;
    case 'e': triggerConfig.enabled = !triggerConfig.enabled;
        Serial.println(triggerConfig.enabled ? F("Edge trigger") : F("Automatic capture")); break;
    case 'n': case 'p': browse(cmd); break;
    case 'd': dumpCapture(); break;
    default: break;
    }
}
void setup() {
    Serial.begin(115200); initSampler(); initDisplay();
    Serial.println(F("l=live r/a=clear+arm t=edge e=edge/auto c0..c7=channel n/p=browse d=CSV"));
}
void loop() {
    while (Serial.available()) handleCommand(static_cast<char>(Serial.read()));
    const uint32_t now = millis();
    if (live && now - lastDisplayUpdateMs >= 100) {
        lastDisplayUpdateMs = now; showLiveState(sampleChannels(), 100000UL);
    }
}
