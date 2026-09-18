#pragma once
// Set to 64 for a 128x64 SSD1306 module. The original I2C sketch used 128x32.
#ifndef OLED_HEIGHT
#define OLED_HEIGHT 32
#endif
#define OLED_SDA 8
#define OLED_SCL 9
#define OLED_ADDRESS 0x3C
#define BUTTON_NEXT 2
#define BUTTON_PREV 3
#define EDGE_QUEUE_SIZE 64
// Software sniffer target: <= 10 kHz with SCL high/low each >= 50 us.
// Faster buses require independent measurement; this is not a 100 kHz guarantee.
