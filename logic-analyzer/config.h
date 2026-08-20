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
