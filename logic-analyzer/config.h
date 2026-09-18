#pragma once

#include <Arduino.h>

#define CH0_PIN 2
#define CH1_PIN 3
#define CH2_PIN 4
#define CH3_PIN 5
#define CH4_PIN 6
#define CH5_PIN 7
#define CH6_PIN A0
#define CH7_PIN A1

#define OLED_SDA 8
#define OLED_SCL 9

#define NUM_CHANNELS 8

static const uint16_t SAMPLE_INTERVAL_US = 10;
static const uint16_t CAPTURE_BUFFER_SIZE = 512;
static const uint16_t PRE_TRIGGER_SAMPLES = 100;
static const uint16_t POST_TRIGGER_SAMPLES = 412;

static const uint8_t DEFAULT_TRIGGER_CHANNEL = 0;
static const bool DEFAULT_TRIGGER_RISING = true;

// POST includes the trigger sample: 100 before + 1 trigger + 411 after.
static_assert(PRE_TRIGGER_SAMPLES + POST_TRIGGER_SAMPLES == CAPTURE_BUFFER_SIZE, "Capture partition mismatch");
static_assert(POST_TRIGGER_SAMPLES > 0, "Post capture includes the trigger sample");
static_assert((CAPTURE_BUFFER_SIZE & (CAPTURE_BUFFER_SIZE - 1)) == 0, "Buffer must be power of two");
static_assert(CAPTURE_BUFFER_SIZE < 32768, "Tick comparisons require a short capture window");
static const uint32_t ARM_TIMEOUT_SAMPLES = 100000UL; // 1 second at 100 kS/s
#ifndef OLED_HEIGHT
#define OLED_HEIGHT 64
#endif
#define OLED_ADDRESS 0x3C
