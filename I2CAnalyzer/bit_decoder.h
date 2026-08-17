#pragma once

#include <stdint.h>

void initBitDecoder();
void addBit(bool bit);
bool byteReady();
uint8_t getByte();
