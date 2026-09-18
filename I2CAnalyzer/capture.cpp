#include "capture.h"
#include "edge_detector.h"
#include "bit_decoder.h"
#include "packet_decoder.h"
#include "ring_buffer.h"
static bool inTransaction = false;
static void (*packetSink)(const Packet&) = pushPacket;
void setPacketSink(void (*sink)(const Packet&)) { packetSink = sink ? sink : pushPacket; }
void resetDecoder() {
    initEdgeDetector(); initBitDecoder(); initPacketDecoder(); inTransaction = false;
}
static void finishPacket(uint8_t flags) {
    // A single rising data bit is also the normal setup for STOP/repeated START.
    if (pendingBitCount() > 1) flags |= PACKET_INCOMPLETE;
    stopPacket(flags);
    if (packetReady()) packetSink(getPacket());
}
void finishDecoder() {
    if (inTransaction) finishPacket(PACKET_INCOMPLETE);
    resetDecoder();
}
void decodeBusState(const BusState& state) {
    switch (detectEdge(state)) {
    case START:
        if (inTransaction) finishPacket(PACKET_RESTART);
        initBitDecoder(); beginPacket(state.atUs); inTransaction = true;
        break;
    case CLOCK_RISE:
        if (inTransaction) {
            addBit(state.sda);
            if (byteReady()) {
                const bool ack = byteAcknowledged();
                feedByte(getByte(), ack);
            }
        }
        break;
    case STOP:
        if (inTransaction) finishPacket(0);
        inTransaction = false; initBitDecoder(); initPacketDecoder();
        break;
    default: break;
    }
}
