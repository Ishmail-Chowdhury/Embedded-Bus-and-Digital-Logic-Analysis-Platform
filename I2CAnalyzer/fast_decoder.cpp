#include "fast_capture.h"
#include "capture.h"
void decodeFastBus(const uint8_t* bytes, uint16_t samples) {
    resetDecoder();
    if (!bytes || !samples || samples > FAST_CAPTURE_BYTES * 2) return;
    // START was detected before the first stored sample. Its exact timestamp
    // is unavailable; use zero and label this approximation in the UI/export.
    decodeBusState(BusState(false, true, 0));
    for (uint16_t i = 0; i < samples; ++i) decodeBusState(fastBusState(bytes, i));
    finishDecoder();
}
