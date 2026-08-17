#include <Arduino.h>
#include <string.h>
#include "bus_sampler.h"
#include "edge_detector.h"
#include "bit_decoder.h"
#include "packet_decoder.h"
#include "ring_buffer.h"
#include "display.h"

static const int BUTTON_NEXT = 2;
static const int BUTTON_PREV = 3;
static const uint8_t OLED_I2C_ADDRESS = 0x3C;

void setup()
{
    Serial.begin(115200);
    initBusSampler();
    initEdgeDetector();
    initBitDecoder();
    initPacketDecoder();
    initRingBuffer();
    initDisplay();

    pinMode(BUTTON_NEXT, INPUT_PULLUP);
    pinMode(BUTTON_PREV, INPUT_PULLUP);
}

void loop()
{
    static int selectedIndex = 0;
    static bool displayDirty = true;
    BusState state = readBus();
    Event event = detectEdge(state);

    switch (event)
    {
        case START:
            initBitDecoder();
            initPacketDecoder();
            break;

        case CLOCK_RISE:
            addBit(state.sda);
            if (byteReady())
            {
                uint8_t byte = getByte();
                feedByte(byte);
            }
            break;

        case STOP:
            stopPacket();
            if (packetReady())
            {
                Packet packet = getPacket();
                if (packet.address != OLED_I2C_ADDRESS)
                {
                    pushPacket(packet);
                    selectedIndex = packetCount() - 1;
                    displayDirty = true;
                }
            }
            break;

        default:
            break;
    }

    int count = packetCount();
    if (count == 0)
    {
        selectedIndex = 0;
    }
    else
    {
        if (digitalRead(BUTTON_NEXT) == LOW)
        {
            int nextIndex = min(count - 1, selectedIndex + 1);
            if (nextIndex != selectedIndex)
            {
                selectedIndex = nextIndex;
                displayDirty = true;
            }
            delay(150);
        }
        if (digitalRead(BUTTON_PREV) == LOW)
        {
            int prevIndex = max(0, selectedIndex - 1);
            if (prevIndex != selectedIndex)
            {
                selectedIndex = prevIndex;
                displayDirty = true;
            }
            delay(150);
        }
        selectedIndex = constrain(selectedIndex, 0, count - 1);
    }

    Packet current;
    if (count == 0 || !getPacket(selectedIndex, current))
    {
        memset(&current, 0, sizeof(current));
    }

    if (displayDirty)
    {
        updateDisplay(count, current, selectedIndex);
        displayDirty = false;
    }

    delay(5);
}
