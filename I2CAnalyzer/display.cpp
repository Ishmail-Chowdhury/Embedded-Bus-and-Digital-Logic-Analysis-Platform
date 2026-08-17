#include "display.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

static const int OLED_RESET = 4;
static const int SCREEN_WIDTH = 128;
static const int SCREEN_HEIGHT = 32;
static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

static char hexBuffer[8];

static String formatHexByte(uint8_t value)
{
    snprintf(hexBuffer, sizeof(hexBuffer), "%02X", value);
    return String(hexBuffer);
}

static String formatDataByte(const Packet& packet)
{
    if (packet.length == 0)
    {
        return "--";
    }
    return formatHexByte(packet.data[0]);
}

static String formatPhase1Line(const Packet& packet)
{
    String result = "STARTADDR 0x";
    result += formatHexByte(packet.address);
    result += packet.read ? " R" : " W";
    result += "DATA 0x";
    result += formatDataByte(packet);
    result += "STOP";
    return result;
}

static String formatPhase2Line(const Packet& packet)
{
    String result = "ADDR 0x";
    result += formatHexByte(packet.address);
    result += packet.read ? " R" : " W";
    result += "DATA 0x";
    result += formatDataByte(packet);
    return result;
}

void initDisplay()
{
    pinMode(OLED_RESET, OUTPUT);
    digitalWrite(OLED_RESET, HIGH);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println("OLED init failed");
        return;
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("I2C Analyzer Ready");
    display.display();
    delay(200);
}

void updateDisplay(int packetCount, const Packet& packet, int selectedIndex)
{
    static int lastShownPacketCount = -1;

    if (packetCount == 0)
    {
        lastShownPacketCount = 0;

        Serial.println("----- OLED OUTPUT -----");
        Serial.println("No packets captured");
        Serial.println("Waiting for I2C traffic");

        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("No packets captured");
        display.println("Waiting for I2C");
        display.println("traffic");
        display.display();
        return;
    }

    String phase1Line = formatPhase1Line(packet);
    String phase2Line = formatPhase2Line(packet);
    String packetLine = "Packet ";
    packetLine += String(selectedIndex + 1);
    packetLine += "/";
    packetLine += String(packetCount);
    bool newCapture = packetCount != lastShownPacketCount;

    Serial.println("----- OLED OUTPUT -----");
    Serial.println(phase1Line);
    Serial.println(phase2Line);
    Serial.println(packetLine);

    display.clearDisplay();
    display.setCursor(0, 0);
    if (newCapture)
    {
        display.println(phase1Line);
    }
    else
    {
        display.println(phase2Line);
    }
    display.println(packetLine);
    display.display();

    lastShownPacketCount = packetCount;
}
