#include <cassert>
#include <cstdio>
#include "capture.h"
#include "edge_detector.h"
#include "bit_decoder.h"
#include "packet_decoder.h"
#include "ring_buffer.h"
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
    assert(packet(5).flags & PACKET_UNSUPPORTED_ADDRESS);
    start(); byte(0x40); pins(true, false); pins(true, true); pins(true, false);
    pins(false, true); pins(false, false); stop();
    assert(packet(6).flags & PACKET_INCOMPLETE);
    initRingBuffer();
    for (uint8_t i = 0; i < 40; ++i) { Packet q = {}; q.address = i; pushPacket(q); }
    assert(packetCount() == 32 && packet(0).address == 8 && packet(31).address == 39);
    assert(!getPacket(-1, p) && !getPacket(32, p));
    puts("PASS I2C: edge classification, ACK/NACK, repeated START, bounds, truncation, ring rollover");
}
