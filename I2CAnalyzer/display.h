#pragma once
#include "packet_decoder.h"
void initDisplay();
void showCaptureStatus(bool capturing, bool overflowed);
void updateDisplay(int count, const Packet& packet, int selectedIndex);
void printPacket(const Packet& packet);
void showFastStatus(bool complete);
