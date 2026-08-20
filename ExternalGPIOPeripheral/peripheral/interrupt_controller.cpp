#include "interrupt_controller.h"
#include "config.h"

InterruptController::InterruptController()
  : pending_(false),
    enabled_(false) {
}

void InterruptController::begin() {
  pinMode(INT_PIN, OUTPUT);
  digitalWrite(INT_PIN, HIGH);
  pending_ = false;
  enabled_ = false;
}

void InterruptController::trigger() {
  if (!enabled_) {
    return;
  }

  pending_ = true;
  digitalWrite(INT_PIN, LOW);
}

void InterruptController::clear() {
  pending_ = false;
  digitalWrite(INT_PIN, HIGH);
}

bool InterruptController::hasPendingInterrupt() const {
  return pending_;
}

void InterruptController::setInterruptEnabled(bool enabled) {
  enabled_ = enabled;
  if (!enabled_) {
    clear();
  }
}
