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

static String formatPacketLine(const Packet& packet)
{
    String result = "STARTADDR 0x";
    result += formatHexByte(packet.address);
    result += packet.read ? " R" : " W";
    result += "DATA 0x";

    if (packet.length == 0)
    {
        result += "--";
    }
    else
    {
        result += formatHexByte(packet.data[0]);
    }

    result += "STOP";
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
    String line1;
    String line2;

    if (packetCount == 0)
    {
        line1 = "No packets captured";
        line2 = "Waiting for I2C traffic";
    }
    else
    {
        String packetLine = formatPacketLine(packet);
        if (packetLine.length() <= 21)
        {
            line1 = packetLine;
            line2 = "Pkt ";
            line2 += String(selectedIndex + 1);
            line2 += "/";
            line2 += String(packetCount);
        }
        else
        {
            line1 = packetLine.substring(0, 21);
            line2 = packetLine.substring(21);
            if (line2.length() > 18)
            {
                line2 = line2.substring(0, 18);
            }
            line2 += " ";
            line2 += "#";
            line2 += String(selectedIndex + 1);
            line2 += "/";
            line2 += String(packetCount);
        }
    }

    Serial.println("----- OLED OUTPUT -----");
    Serial.println(line1);
    Serial.println(line2);

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(line1);
    display.println(line2);
    display.display();
}

bool displayNextRequested()
{
    return false;
}

bool displayPrevRequested()
{
    return false;
}
