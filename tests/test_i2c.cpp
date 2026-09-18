#include <cassert>
#include <cstdio>
#include "capture.h"
#include "edge_detector.h"
#include "bit_decoder.h"
#include "packet_decoder.h"
#include "ring_buffer.h"
#include "config.h"
#include "bus_filter.h"
static void pins(bool sda, bool scl) { decodeBusState({sda, scl}); }
static void start() { pins(true, false); pins(true, true); pins(false, true); pins(false, false); }
static void stop() { pins(false, false); pins(false, true); pins(true, true); }
static void byte(uint8_t value, bool ack = true) {
    for (uint8_t mask = 0x80; mask; mask >>= 1) {
        bool bit = value & mask; pins(bit, false); pins(bit, true); pins(bit, false);
    }
    pins(!ack, false); pins(!ack, true); pins(!ack, false);
}
static Packet packet(int index) { Packet p; assert(getPacket(index, p)); return p; }
int main() {
    initEdgeDetector();
    assert(detectEdge({false, false}) == CLOCK_FALL);
    assert(detectEdge({true, true}) == CLOCK_RISE); // No false STOP on simultaneous sampled changes.
    initBitDecoder();
    for (int i = 0; i < 8; ++i) addBit(i % 2);
    assert(!byteReady()); addBit(false);
    assert(byteReady() && byteAcknowledged() && getByte() == 0x55);
    initRingBuffer(); resetDecoder();
    byte(0x40); stop(); assert(packetCount() == 0); // Ignore attach-mid-transaction.
    start(); byte(0x40); byte(0x0A);
    start(); byte(0x41); byte(0x42, false); stop(); // Repeated START read.
    assert(packetCount() == 2);
    Packet p = packet(0);
    assert(p.address == 0x20 && !p.read && p.data[0] == 0x0A && p.flags == PACKET_RESTART);
    p = packet(1);
    assert(p.read && p.data[0] == 0x42 && p.flags == PACKET_NACK && p.nackIndex == 1);
    start(); byte(0x78); byte(0x55); stop(); // External 0x3c is legitimate data now.
    assert(packet(2).address == 0x3C);
    start(); byte(0x70, false); stop();
    assert(packet(3).nackIndex == 0 && packet(3).length == 0);
    start(); byte(0x40);
    for (uint8_t i = 0; i < 20; ++i) byte(i);
    stop(); p = packet(4);
    assert(p.length == 16 && p.data[15] == 15 && p.flags == PACKET_TRUNCATED);
    start(); byte(0xF0); byte(0x12); stop();
    assert(packet(5).tenBit && packet(5).address == 0x12 && packet(5).length == 0);
    start(); byte(0x40); pins(true, false); pins(true, true); pins(true, false);
    pins(false, true); pins(false, false); stop();
    assert(packet(6).flags & PACKET_INCOMPLETE);
    initRingBuffer();
    for (uint8_t i = 0; i < 40; ++i) { Packet q = {}; q.address = i; pushPacket(q); }
    assert(packetCount() == PACKET_HISTORY_SIZE && packet(0).address == 40 - PACKET_HISTORY_SIZE && packet(PACKET_HISTORY_SIZE - 1).address == 39);
    assert(!getPacket(-1, p) && !getPacket(PACKET_HISTORY_SIZE, p));
    initRingBuffer(); resetDecoder();
    pins(true, false); pins(true, true); decodeBusState({false, true, 123456}); pins(false, false);
    byte(0xF4); byte(0xAA); start(); byte(0xF5); byte(0x5A, false); stop();
    assert(packet(0).address == 0x2AA && packet(0).tenBit && packet(0).startUs == 123456);
    assert(packet(1).address == 0x2AA && packet(1).read && packet(1).data[0] == 0x5A);
    start(); byte(0xF5); byte(0, false); stop();
    assert(packet(2).flags & PACKET_ADDRESS_ERROR); // STOP clears 10-bit selection.
    start(); byte(0xF6); byte(0xFF, false); start(); byte(0xF7); stop();
    assert(packet(3).address == 0x3FF && packet(3).nackIndex == 0);
    assert(packet(4).flags & PACKET_ADDRESS_ERROR); // NACKed address cannot select target.
    start(); byte(0xF0); stop(); assert(packet(5).flags & PACKET_INCOMPLETE);
    BusFilter filter; BusState accepted;
    filter.reset(8);
    assert(!filter.push({false, true, 100}, accepted));
    assert(!filter.push({true, true, 104}, accepted));
    assert(!filter.settle(120, accepted)); // Reject 4 us SDA glitch.
    assert(!filter.push({false, true, 200}, accepted));
    assert(!filter.settle(207, accepted));
    assert(filter.settle(208, accepted) && accepted.atUs == 200 && !accepted.sda);
    filter.reset(8); filter.push({false, true, 0xFFFFFFFC}, accepted);
    assert(filter.settle(4, accepted) && accepted.atUs == 0xFFFFFFFC);
    filter.reset(0); assert(filter.push({false, false, 1}, accepted));
    puts("PASS I2C: edge classification, ACK/NACK, repeated START, bounds, truncation, ring rollover");
}
