#include <Arduino.h>
#include "config.h"
#include "sampler.h"
#include "trigger.h"
#include "capture_session.h"
#include "display.h"
#include "analog_capture.h"
static uint16_t analogCount = 0;
static CaptureSession session;
static TriggerConfig triggerConfig = {DEFAULT_TRIGGER_CHANNEL, DEFAULT_TRIGGER_RISING, true};
static bool live = true, complete = false, awaitingChannel = false;
static uint16_t browseIndex = 0;
static bool waveform = false;
static uint8_t samplesPerPixel = 4, firstChannel = 0;
static uint32_t lastDisplayUpdateMs = 0;

static void armCapture() {
    analogCount = 0;
    showArmedStatus(sampleChannels(), 0, 100000UL, triggerConfig);
    Serial.println(F("ARMED: up to 1 second; do not send commands during capture"));
    Serial.flush();
    const CaptureResult result = captureBurst(session, triggerConfig);
    // UART cannot service traffic during the burst; discard any pending input.
    while (Serial.available()) Serial.read();
    complete = result == CAPTURE_OK; live = false; waveform = false;
    if (!complete) {
        Serial.println(result == CAPTURE_TIMEOUT ? F("TIMEOUT: no trigger; r to retry") : F("TIMING OVERRUN: capture discarded"));
        showCaptureError(result == CAPTURE_TIMEOUT);
        return;
    }
    browseIndex = static_cast<uint16_t>(session.buffer.findFirstTickAtOrAfter(session.triggerTick));
    showCaptureSummary(session.buffer.size(), browseIndex, session.buffer.size() - browseIndex, triggerConfig);
    Serial.println(F("CAPTURE COMPLETE: 512 samples, 100 before / 412 including trigger"));
}
static void renderWaveform() {
    if (!complete) return;
    showWaveform(session.buffer, browseIndex, samplesPerPixel, firstChannel, session.triggerTick);
    Serial.print(F("Wave start=")); Serial.print(browseIndex);
    Serial.print(F(" us/pixel=")); Serial.println(uint16_t(samplesPerPixel) * SAMPLE_INTERVAL_US);
}
static void browse(char cmd) {
    if (!complete) return;
    const uint16_t step = waveform ? uint16_t(samplesPerPixel) * 30 : 1;
    if (cmd == 'n') browseIndex = (browseIndex + step < session.buffer.size()) ? browseIndex + step : session.buffer.size() - 1;
    if (cmd == 'p') browseIndex = browseIndex > step ? browseIndex - step : 0;
    if (waveform) { renderWaveform(); return; }
    Sample sample;
    if (session.buffer.get(browseIndex, sample)) showSampleDetail(browseIndex, session.buffer.size(), sample, session.triggerTick);
}
static void measureSelectedPulse() {
    PulseMeasurement pulse;
    if (!complete || !measurePulse(session.buffer, triggerConfig.channel, browseIndex, pulse)) return;
    waveform = false;
    showPulse(pulse, triggerConfig.channel);
    Serial.print(F("Pulse CH")); Serial.print(triggerConfig.channel);
    Serial.print(pulse.high ? F(" HIGH ") : F(" LOW "));
    Serial.print((pulse.leftClipped || pulse.rightClipped) ? F(">= ") : F("~ "));
    Serial.print(pulse.widthUs); Serial.println(F(" us (10 us sampling resolution)"));
}
static void captureAnalogMode() {
    complete = live = waveform = false;
    Serial.println(F("A2 capture: 256 samples, AVCC reference, 104 us/sample")); Serial.flush();
    analogCount = captureAnalog(session.buffer.scratchData(), CAPTURE_BUFFER_SIZE);
    while (Serial.available()) Serial.read();
    if (analogCount) showAnalog(session.buffer.rawData(), analogCount);
    else { showCaptureError(false); Serial.println(F("ADC capture failed")); }
}
static void dumpCapture() {
    if (analogCount) {
        Serial.println(F("index,relative_us,adc10"));
        for (uint16_t i = 0; i < analogCount; ++i) {
            Serial.print(i); Serial.print(','); Serial.print(uint32_t(i) * ANALOG_INTERVAL_US);
            Serial.print(','); Serial.println(analogValue(session.buffer.rawData(), i));
        }
        return;
    }
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
    case 'l': live = true; complete = false; analogCount = 0; break;
    case 't': triggerConfig.risingEdge = !triggerConfig.risingEdge;
        Serial.println(triggerConfig.risingEdge ? F("Rising edge") : F("Falling edge")); break;
    case 'e': triggerConfig.enabled = !triggerConfig.enabled;
        Serial.println(triggerConfig.enabled ? F("Edge trigger") : F("Automatic capture")); break;
    case 'n': case 'p': browse(cmd); break;
    case 'o': captureAnalogMode(); break;
    case 'w': if (analogCount) { showAnalog(session.buffer.rawData(), analogCount); break; } if (complete) { waveform = !waveform; if (waveform) renderWaveform(); else browse(' '); } break;
    case '+': if (samplesPerPixel > 1) samplesPerPixel /= 2; if (waveform) renderWaveform(); break;
    case '-': if (samplesPerPixel < 8) samplesPerPixel *= 2; if (waveform) renderWaveform(); break;
    case 'v': firstChannel = OLED_HEIGHT == 32 ? (firstChannel ^ 4) : 0; if (waveform) renderWaveform(); break;
    case 'm': measureSelectedPulse(); break;
    case 'd': dumpCapture(); break;
    default: break;
    }
}
void setup() {
    Serial.begin(115200); initSampler(); initDisplay();
    Serial.println(F("l=live r/a=clear+arm t=edge e=edge/auto c0..c7=channel n/p=browse d=CSV w=wave +/-=zoom v=bank m=pulse o=A2 ADC"));
}
void loop() {
    while (Serial.available()) handleCommand(static_cast<char>(Serial.read()));
    const uint32_t now = millis();
    if (live && now - lastDisplayUpdateMs >= 100) {
        lastDisplayUpdateMs = now; showLiveState(sampleChannels(), 100000UL);
    }
}
