#pragma once
#include "bus_sampler.h"
#include "packet_decoder.h"
void resetDecoder();
void decodeBusState(const BusState& state);
void setPacketSink(void (*sink)(const Packet&));
void finishDecoder();
