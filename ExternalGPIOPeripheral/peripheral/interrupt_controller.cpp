#include "interrupt_controller.h"
#include "config.h"
void InterruptController::begin() {
    digitalWrite(INT_PIN, LOW);
    pinMode(INT_PIN, INPUT);
    pending_ = false;
}
void InterruptController::setPending(bool pending) {
    pending_ = pending;
    // The host supplies the pull-up; never drive the line high.
    pinMode(INT_PIN, pending ? OUTPUT : INPUT);
}
bool InterruptController::hasPendingInterrupt() const { return pending_; }
