#pragma once

#include <Arduino.h>
#include "capture_buffer.h"
#include "trigger.h"

bool initDisplay();

void showLiveState(uint8_t state, uint32_t sampleRateHz);
void showArmedStatus(uint8_t state, uint16_t bufferedSamples, uint32_t sampleRateHz, const TriggerConfig& trigger);
void showCaptureSummary(uint16_t totalSamples, uint16_t preSamples, uint16_t postSamples, const TriggerConfig& trigger);
void showSampleDetail(uint16_t sampleIndex, uint16_t totalSamples, const Sample& sample, uint16_t triggerTick);

void showCaptureError(bool timeout);

#include "measurements.h"
void showWaveform(const CaptureBuffer& capture, uint16_t start, uint8_t samplesPerPixel, uint8_t firstChannel, uint16_t triggerTick);
void showPulse(const PulseMeasurement& pulse, uint8_t channel);
