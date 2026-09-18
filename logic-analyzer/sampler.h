#pragma once
#include <Arduino.h>
#include "capture_session.h"
void initSampler();
uint8_t sampleChannels();
enum CaptureResult { CAPTURE_OK, CAPTURE_TIMEOUT, CAPTURE_OVERRUN };
CaptureResult captureBurst(CaptureSession& session, const TriggerConfig& config);
