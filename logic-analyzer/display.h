#pragma once

#include <Arduino.h>
#include "capture_buffer.h"
#include "trigger.h"

bool initDisplay();

void showLiveState(uint8_t state, uint16_t sampleRateHz);
void showArmedStatus(uint8_t state, uint16_t bufferedSamples, uint16_t sampleRateHz, const TriggerConfig& trigger);
void showCaptureSummary(uint16_t totalSamples, uint16_t preSamples, uint16_t postSamples, const TriggerConfig& trigger);
void showSampleDetail(uint16_t sampleIndex, uint16_t totalSamples, const Sample& sample, uint16_t triggerTick);
