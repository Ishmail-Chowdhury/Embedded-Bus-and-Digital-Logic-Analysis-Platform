#pragma once

#include "packet_decoder.h"

void initRingBuffer();
void pushPacket(const Packet& packet);
int packetCount();
bool getPacket(int index, Packet& out);
uint8_t* packetScratch();
const uint8_t* rawBusCapture();
