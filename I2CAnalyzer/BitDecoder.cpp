#include "bit_decoder.h"

static uint8_t currentByte = 0;
static uint8_t bitCount = 0;
static bool ready = false;
static bool skipAckBit = false;

void initBitDecoder()
{
    currentByte = 0;
    bitCount = 0;
    ready = false;
    skipAckBit = false;
}

void addBit(bool bit)
{
    if (skipAckBit)
    {
        skipAckBit = false;
        return;
    }

    currentByte = (currentByte << 1) | (bit ? 1 : 0);
    bitCount++;
    if (bitCount == 8)
    {
        ready = true;
        skipAckBit = true;
    }
}

bool byteReady()
{
    return ready;
}

uint8_t getByte()
{
    uint8_t result = currentByte;
    currentByte = 0;
    bitCount = 0;
    ready = false;
    return result;
}
