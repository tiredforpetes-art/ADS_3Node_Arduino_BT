#ifndef DISPLAY_H
#define DISPLAY_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_ADDR 0x3C

extern Adafruit_SSD1306 display;

void display_init();
void display_status(float voltage, uint32_t timestamp, uint32_t frameCount, bool alive);

#endif