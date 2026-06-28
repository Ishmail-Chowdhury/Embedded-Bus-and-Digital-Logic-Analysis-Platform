#include <Arduino.h>
#include "bus_sampler.h"
#include "edge_detector.h"
#include "bit_decoder.h"
#include "packet_decoder.h"
#include "ring_buffer.h"
#include "display.h"

static const int BUTTON_NEXT = 2;
static const int BUTTON_PREV = 3;

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
                pushPacket(packet);
                selectedIndex = packetCount() - 1;
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
            selectedIndex = min(count - 1, selectedIndex + 1);
            delay(150);
        }
        if (digitalRead(BUTTON_PREV) == LOW)
        {
            selectedIndex = max(0, selectedIndex - 1);
            delay(150);
        }
        selectedIndex = constrain(selectedIndex, 0, count - 1);
    }

    Packet current;
    if (count == 0 || !getPacket(selectedIndex, current))
    {
        memset(&current, 0, sizeof(current));
    }
    updateDisplay(count, current, selectedIndex);
    delay(5);
}
