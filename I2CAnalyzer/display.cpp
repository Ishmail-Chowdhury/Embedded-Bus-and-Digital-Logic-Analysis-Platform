#include "display.h"
#include "config.h"
#include <Arduino.h>
#include <U8x8lib.h>
#if OLED_HEIGHT == 32
static U8X8_SSD1306_128X32_UNIVISION_SW_I2C display(OLED_SCL, OLED_SDA, U8X8_PIN_NONE);
#elif OLED_HEIGHT == 64
static U8X8_SSD1306_128X64_NONAME_SW_I2C display(OLED_SCL, OLED_SDA, U8X8_PIN_NONE);
#else
#error "OLED_HEIGHT must be 32 or 64"
#endif
void initDisplay() {
    display.setBusClock(100000);
    display.setI2CAddress(OLED_ADDRESS << 1);
    display.begin(); display.setFont(u8x8_font_chroma48medium8_r);
}
void showCaptureStatus(bool capturing, bool overflowed) {
    display.clear();
    display.drawString(0, 0, capturing ? "CAPTURING <=10k" : "CAPTURE PAUSED");
    display.drawString(0, 1, overflowed ? "EDGE LOSS!" : "r=arm s=pause");
    display.drawString(0, 2, "n/p browse");
}
void updateDisplay(int count, const Packet& packet, int selectedIndex) {
    display.clear();
    if (!count) { display.drawString(0, 0, "No packets"); return; }
    char line[17];
    snprintf(line, sizeof(line), "%d/%d %03X %c N%u", selectedIndex + 1, count,
             packet.address, packet.read ? 'R' : 'W', packet.length);
    display.drawString(0, 0, line);
    for (uint8_t row = 0; row < 2; ++row) {
        char data[17] = {};
        for (uint8_t i = 0; i < 4 && row * 4 + i < packet.length; ++i)
            snprintf(data + i * 3, 4, "%02X ", packet.data[row * 4 + i]);
        display.drawString(0, row + 1, data);
    }
    snprintf(line, sizeof(line), "FLAGS %02X NAK %u", packet.flags, packet.nackIndex);
    display.drawString(0, 3, line);
    Serial.print(F("START_us=")); Serial.print(packet.startUs);
    Serial.print(packet.tenBit ? F(" 10-bit ") : F(" 7-bit "));
    Serial.print(F("ADDR=0x")); Serial.print(packet.address, HEX);
    Serial.print(packet.read ? F(" R ") : F(" W "));
    for (uint8_t i = 0; i < packet.length; ++i) { Serial.print(packet.data[i], HEX); Serial.print(' '); }
    Serial.print(F("FLAGS=0x")); Serial.print(packet.flags, HEX);
    Serial.print(F(" NACK index=")); Serial.println(packet.nackIndex);
}
