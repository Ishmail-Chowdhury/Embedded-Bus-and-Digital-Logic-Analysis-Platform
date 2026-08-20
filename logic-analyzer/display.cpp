#include "display.h"
#include "config.h"
#include <U8x8lib.h>

static U8X8_SSD1306_128X64_NONAME_SW_I2C display(OLED_SCL, OLED_SDA, U8X8_PIN_NONE);

static void toBinaryString(uint8_t value, char* out)
{
    for (int i = 7; i >= 0; i--)
    {
        out[7 - i] = ((value >> i) & 0x01) ? '1' : '0';
    }
    out[8] = '\0';
}

bool initDisplay()
{
    display.begin();
    display.setFlipMode(0);
    display.clear();
    display.setFont(u8x8_font_chroma48medium8_r);
    display.drawString(0, 0, "DIGITAL ANALYZER");
    display.drawString(0, 2, "Phase 1-3 Ready");
    display.drawString(0, 4, "Serial: r/n/p/t/e");
    return true;
}

void showLiveState(uint8_t state, uint16_t sampleRateHz)
{
    char bin[9];
    char line2[17];
    char line3[17];

    toBinaryString(state, bin);
    snprintf(line2, sizeof(line2), "CH 76543210");
    snprintf(line3, sizeof(line3), "   %s", bin);

    display.clearLine(0);
    display.clearLine(1);
    display.clearLine(2);
    display.clearLine(3);
    display.clearLine(4);
    display.drawString(0, 0, "LIVE MONITOR");
    display.drawString(0, 2, line2);
    display.drawString(0, 3, line3);

    char line4[17];
    snprintf(line4, sizeof(line4), "%u kS/s", sampleRateHz / 1000);
    display.drawString(0, 5, line4);
}

void showArmedStatus(uint8_t state, uint16_t bufferedSamples, uint16_t sampleRateHz, const TriggerConfig& trigger)
{
    char bin[9];
    char line2[17];
    char line3[17];
    char line4[17];
    char line5[17];

    toBinaryString(state, bin);

    snprintf(line2, sizeof(line2), "CH 76543210");
    snprintf(line3, sizeof(line3), "   %s", bin);
    snprintf(line4, sizeof(line4), "BUF %u/%u", bufferedSamples, CAPTURE_BUFFER_SIZE);
    snprintf(line5, sizeof(line5), "TRG CH%u %c", trigger.channel, trigger.risingEdge ? '^' : 'v');

    display.clear();
    display.drawString(0, 0, "ARMED");
    display.drawString(0, 1, line5);
    display.drawString(0, 2, line2);
    display.drawString(0, 3, line3);
    display.drawString(0, 5, line4);

    char line6[17];
    snprintf(line6, sizeof(line6), "%u kS/s", sampleRateHz / 1000);
    display.drawString(0, 6, line6);
}

void showCaptureSummary(uint16_t totalSamples, uint16_t preSamples, uint16_t postSamples, const TriggerConfig& trigger)
{
    char line2[17];
    char line3[17];
    char line4[17];
    char line5[17];

    snprintf(line2, sizeof(line2), "TRG CH%u %c", trigger.channel, trigger.risingEdge ? '^' : 'v');
    snprintf(line3, sizeof(line3), "PRE %u", preSamples);
    snprintf(line4, sizeof(line4), "POST %u", postSamples);
    snprintf(line5, sizeof(line5), "TOTAL %u", totalSamples);

    display.clear();
    display.drawString(0, 0, "CAPTURE COMPLETE");
    display.drawString(0, 2, line2);
    display.drawString(0, 3, line3);
    display.drawString(0, 4, line4);
    display.drawString(0, 5, line5);
    display.drawString(0, 7, "n/p browse, r rearm");
}

void showSampleDetail(uint16_t sampleIndex, uint16_t totalSamples, const Sample& sample, uint16_t triggerTick)
{
    char bin[9];
    char line1[17];
    char line2[17];
    char line3[17];
    char line4[17];

    toBinaryString(sample.state, bin);

    const int16_t deltaTick = static_cast<int16_t>(sample.tick - triggerTick);
    const int32_t deltaUs = static_cast<int32_t>(deltaTick) * SAMPLE_INTERVAL_US;

    snprintf(line1, sizeof(line1), "IDX %u/%u", sampleIndex + 1, totalSamples);
    snprintf(line2, sizeof(line2), "STATE %s", bin);
    snprintf(line3, sizeof(line3), "HEX 0x%02X", sample.state);
    snprintf(line4, sizeof(line4), "dT %ld us", static_cast<long>(deltaUs));

    display.clear();
    display.drawString(0, 0, "SAMPLE VIEW");
    display.drawString(0, 2, line1);
    display.drawString(0, 3, line2);
    display.drawString(0, 4, line3);
    display.drawString(0, 5, line4);
    display.drawString(0, 7, "n/p next/prev");
}
