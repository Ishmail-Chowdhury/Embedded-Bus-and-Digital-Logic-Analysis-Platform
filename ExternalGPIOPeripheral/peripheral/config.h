#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#define PERIPHERAL_ADDRESS 0x20
#define DEVICE_ID_VALUE 0x42

#define REG_GPIO0_OUTPUT      0x00
#define REG_GPIO1_OUTPUT      0x01
#define REG_GPIO0_INPUT       0x02
#define REG_GPIO1_INPUT       0x03
#define REG_DIRECTION0        0x04
#define REG_DIRECTION1        0x05
#define REG_STATUS            0x06
#define REG_CONTROL           0x07
#define REG_INTERRUPT_STATUS  0x08
#define REG_INTERRUPT_ENABLE  0x09
#define REG_DEVICE_ID         0x0A

#define REGISTER_COUNT        0x10

#define STATUS_ENABLED        0x01
#define STATUS_INTERRUPT      0x02
#define STATUS_ERROR          0x04

#define INT_PIN               2

#endif
