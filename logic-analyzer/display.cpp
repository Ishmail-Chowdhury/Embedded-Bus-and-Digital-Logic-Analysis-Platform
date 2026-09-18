#include "display.h"
#include "config.h"
#include <U8x8lib.h>
#if OLED_HEIGHT == 64
static U8X8_SSD1306_128X64_NONAME_SW_I2C display(OLED_SCL, OLED_SDA, U8X8_PIN_NONE);
#elif OLED_HEIGHT == 32
static U8X8_SSD1306_128X32_UNIVISION_SW_I2C display(OLED_SCL, OLED_SDA, U8X8_PIN_NONE);
#else
#error "OLED_HEIGHT must be 32 or 64"
#endif
static void binary(uint8_t value, char* out) {
    for (uint8_t i = 0; i < 8; ++i) out[i] = value & (0x80 >> i) ? '1' : '0';
    out[8] = 0;
}
bool initDisplay() {
    display.setBusClock(100000);
    display.setI2CAddress(OLED_ADDRESS << 1); display.begin();
    display.setFont(u8x8_font_chroma48medium8_r); display.clear();
    return true;
}
void showLiveState(uint8_t state, uint32_t sampleRateHz) {
    (void)sampleRateHz;
    char bits[9]; binary(state, bits); display.clear();
    display.drawString(0, 0, "LIVE ~10 Hz UI");
    display.drawString(0, 1, "CH 76543210");
    display.drawString(3, 2, bits);
    display.drawString(0, 3, "r=100 kS/s burst");
}
void showArmedStatus(uint8_t state, uint16_t bufferedSamples, uint32_t sampleRateHz, const TriggerConfig& trigger) {
    (void)state; (void)bufferedSamples;
    char line[17]; display.clear();
    snprintf(line, sizeof(line), "%lu kS/s 1s max", static_cast<unsigned long>(sampleRateHz / 1000));
    display.drawString(0, 0, "ARMED"); display.drawString(0, 1, line);
    snprintf(line, sizeof(line), "CH%u %c %s", trigger.channel, trigger.risingEdge ? '^' : 'v', trigger.enabled ? "edge" : "auto");
    display.drawString(0, 2, line);
}
void showCaptureSummary(uint16_t totalSamples, uint16_t preSamples, uint16_t postSamples, const TriggerConfig& trigger) {
    (void)trigger;
    char line[17]; display.clear();
    display.drawString(0, 0, "CAPTURE COMPLETE");
    snprintf(line, sizeof(line), "PRE%u POST%u", preSamples, postSamples); display.drawString(0, 1, line);
    snprintf(line, sizeof(line), "TOTAL %u", totalSamples); display.drawString(0, 2, line);
    display.drawString(0, 3, "n/p view d=CSV");
}
void showSampleDetail(uint16_t sampleIndex, uint16_t totalSamples, const Sample& sample, uint16_t triggerTick) {
    char line[17], bits[9]; binary(sample.state, bits); display.clear();
    snprintf(line, sizeof(line), "IDX %u/%u", sampleIndex + 1, totalSamples); display.drawString(0, 0, line);
    display.drawString(0, 1, bits);
    const int32_t deltaUs = static_cast<int16_t>(sample.tick - triggerTick) * static_cast<int32_t>(SAMPLE_INTERVAL_US);
    snprintf(line, sizeof(line), "dT %ld us", static_cast<long>(deltaUs)); display.drawString(0, 2, line);
    display.drawString(0, 3, "n/p browse r=arm");
}

void showCaptureError(bool timeout) {
    display.clear();
    display.drawString(0, 0, timeout ? "TRIGGER TIMEOUT" : "TIMING OVERRUN");
    display.drawString(0, 1, "No valid capture");
    display.drawString(0, 3, "r=retry l=live");
}
