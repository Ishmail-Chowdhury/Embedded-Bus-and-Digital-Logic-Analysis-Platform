#include <Arduino.h>
#include "config.h"
#include "sampler.h"
#include "trigger.h"
#include "capture_buffer.h"
#include "display.h"

enum AnalyzerMode
{
    MODE_LIVE,
    MODE_ARMED,
    MODE_POST_TRIGGER,
    MODE_COMPLETE
};

static CaptureBuffer capture;
static TriggerConfig triggerConfig = {DEFAULT_TRIGGER_CHANNEL, DEFAULT_TRIGGER_RISING, true};
static AnalyzerMode mode = MODE_LIVE;

static uint8_t previousState = 0;
static uint8_t currentState = 0;

static uint16_t sampleTick = 0;
static uint16_t triggerTick = 0;
static uint16_t postRemaining = 0;
static uint16_t browseIndex = 0;

static uint32_t nextSampleAtUs = 0;
static uint32_t lastDisplayUpdateMs = 0;

static void printHelp()
{
    Serial.println("Commands:");
    Serial.println("  a = arm capture");
    Serial.println("  l = live monitor");
    Serial.println("  r = re-arm and clear buffer");
    Serial.println("  n = next sample (when capture complete)");
    Serial.println("  p = previous sample (when capture complete)");
    Serial.println("  t = toggle trigger edge rising/falling");
    Serial.println("  c0..c7 = set trigger channel");
    Serial.println("  e = trigger enable/disable");
}

static void printStateLine(const char* tag, uint8_t state)
{
    char binary[9];
    for (int i = 7; i >= 0; i--)
    {
        binary[7 - i] = ((state >> i) & 0x01) ? '1' : '0';
    }
    binary[8] = '\0';

    Serial.print(tag);
    Serial.print(" state=");
    Serial.print(binary);
    Serial.print(" hex=0x");
    if (state < 0x10)
    {
        Serial.print('0');
    }
    Serial.println(state, HEX);
}

static void armCapture(bool clearBuffer)
{
    if (clearBuffer)
    {
        capture.clear();
        sampleTick = 0;
    }

    postRemaining = POST_TRIGGER_SAMPLES;
    browseIndex = 0;

    mode = MODE_ARMED;

    Serial.print("ARMED trigger CH");
    Serial.print(triggerConfig.channel);
    Serial.print(triggerConfig.risingEdge ? " rising" : " falling");
    Serial.print(" enabled=");
    Serial.println(triggerConfig.enabled ? "yes" : "no");
}

static void handleCompleteBrowseCommand(char cmd)
{
    const uint16_t total = capture.size();
    if (total == 0)
    {
        return;
    }

    if (cmd == 'n' && browseIndex + 1 < total)
    {
        browseIndex++;
    }
    else if (cmd == 'p' && browseIndex > 0)
    {
        browseIndex--;
    }

    Sample s;
    if (capture.get(browseIndex, s))
    {
        showSampleDetail(browseIndex, total, s, triggerTick);
        printStateLine("BROWSE", s.state);
    }
}

static void handleCommand(char cmd)
{
    if (cmd == 'a')
    {
        armCapture(false);
        return;
    }

    if (cmd == 'l')
    {
        mode = MODE_LIVE;
        Serial.println("Switched to LIVE mode");
        return;
    }

    if (cmd == 'r')
    {
        armCapture(true);
        return;
    }

    if (cmd == 't')
    {
        triggerConfig.risingEdge = !triggerConfig.risingEdge;
        Serial.print("Trigger edge now ");
        Serial.println(triggerConfig.risingEdge ? "rising" : "falling");
        return;
    }

    if (cmd == 'e')
    {
        triggerConfig.enabled = !triggerConfig.enabled;
        Serial.print("Trigger enabled = ");
        Serial.println(triggerConfig.enabled ? "yes" : "no");
        return;
    }

    if (cmd >= '0' && cmd <= '7')
    {
        triggerConfig.channel = static_cast<uint8_t>(cmd - '0');
        Serial.print("Trigger channel set to CH");
        Serial.println(triggerConfig.channel);
        return;
    }

    if (mode == MODE_COMPLETE)
    {
        handleCompleteBrowseCommand(cmd);
    }
}

static void readSerialCommands()
{
    while (Serial.available() > 0)
    {
        const char cmd = static_cast<char>(Serial.read());
        if (cmd == '\n' || cmd == '\r' || cmd == ' ')
        {
            continue;
        }

        if (cmd == 'c')
        {
            while (Serial.available() == 0)
            {
            }
            const char channelChar = static_cast<char>(Serial.read());
            if (channelChar >= '0' && channelChar <= '7')
            {
                handleCommand(channelChar);
            }
            continue;
        }

        handleCommand(cmd);
    }
}

static void processSample()
{
    currentState = sampleChannels();

    Sample s;
    s.tick = sampleTick++;
    s.state = currentState;
    capture.push(s);

    if (mode == MODE_ARMED)
    {
        if (triggerDetected(currentState, previousState, triggerConfig))
        {
            mode = MODE_POST_TRIGGER;
            triggerTick = s.tick;
            postRemaining = POST_TRIGGER_SAMPLES;

            Serial.print("TRIGGER detected on CH");
            Serial.print(triggerConfig.channel);
            Serial.print(triggerConfig.risingEdge ? " rising" : " falling");
            Serial.print(" at tick=");
            Serial.println(triggerTick);
        }
    }
    else if (mode == MODE_POST_TRIGGER)
    {
        if (postRemaining > 0)
        {
            postRemaining--;
        }

        if (postRemaining == 0)
        {
            mode = MODE_COMPLETE;
            int16_t trigIndex = capture.findFirstTickAtOrAfter(triggerTick);
            if (trigIndex < 0)
            {
                browseIndex = 0;
            }
            else
            {
                browseIndex = static_cast<uint16_t>(trigIndex);
            }

            const uint16_t total = capture.size();
            uint16_t preSamples = 0;
            if (browseIndex <= total)
            {
                preSamples = browseIndex;
            }
            uint16_t postSamples = 0;
            if (total > browseIndex)
            {
                postSamples = total - browseIndex;
            }

            showCaptureSummary(total, preSamples, postSamples, triggerConfig);

            Serial.println("CAPTURE COMPLETE");
            Serial.print("Samples total=");
            Serial.println(total);
            Serial.print("Pre-trigger samples=");
            Serial.println(preSamples);
            Serial.print("Post-trigger samples=");
            Serial.println(postSamples);

            Sample view;
            if (capture.get(browseIndex, view))
            {
                showSampleDetail(browseIndex, total, view, triggerTick);
                printStateLine("TRIGGER SAMPLE", view.state);
            }
        }
    }

    previousState = currentState;
}

static void updateDisplayIfNeeded()
{
    const uint32_t nowMs = millis();
    if (nowMs - lastDisplayUpdateMs < 100)
    {
        return;
    }
    lastDisplayUpdateMs = nowMs;

    if (mode == MODE_LIVE)
    {
        showLiveState(currentState, 1000000UL / SAMPLE_INTERVAL_US);
    }
    else if (mode == MODE_ARMED || mode == MODE_POST_TRIGGER)
    {
        showArmedStatus(currentState, capture.size(), 1000000UL / SAMPLE_INTERVAL_US, triggerConfig);
    }
}

void setup()
{
    Serial.begin(115200);
    initSampler();
    initDisplay();

    previousState = sampleChannels();
    currentState = previousState;

    printHelp();
    Serial.println("Phase 1: live monitor (mode l)");
    Serial.println("Phase 2/3: arm with command a or r");

    mode = MODE_LIVE;
    nextSampleAtUs = micros();
}

void loop()
{
    readSerialCommands();

    const uint32_t nowUs = micros();
    if ((int32_t)(nowUs - nextSampleAtUs) >= 0)
    {
        nextSampleAtUs += SAMPLE_INTERVAL_US;

        if (mode != MODE_COMPLETE)
        {
            processSample();
        }
    }

    updateDisplayIfNeeded();
}
