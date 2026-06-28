#pragma once

#include "packet_decoder.h"

void initDisplay();
void updateDisplay(int packetCount, const Packet& packet, int selectedIndex);
bool displayNextRequested();
bool displayPrevRequested();
