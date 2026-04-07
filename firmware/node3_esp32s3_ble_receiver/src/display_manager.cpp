#include "display_manager.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

bool DisplayManager::begin() {
  Wire.begin(D4, D5);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    available_ = false;
    return false;
  }

  available_ = true;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("NODE3 BLE RX");
  display.println("Display OK");
  display.display();

  return true;
}

void DisplayManager::setBleStatus(bool connected) {
  bleConnected_ = connected;
}

void DisplayManager::updateFrame(uint32_t timestampMs, float currentA) {
  timestampMs_ = timestampMs;
  currentA_ = currentA;
}

void DisplayManager::setFramesOk(uint32_t frames) {
  framesOk_ = frames;
}

void DisplayManager::setMessage(const char* msg) {
  message_ = msg;
}

void DisplayManager::refresh() {
  if (!available_) return;

  display.clearDisplay();
  display.setCursor(0, 0);

  display.println("NODE3 BLE RX");

  display.print("BLE: ");
  display.println(bleConnected_ ? "ON" : "OFF");

  display.print("Frames: ");
  display.println(framesOk_);

  display.print("t: ");
  display.print(timestampMs_ / 1000.0f, 3);
  display.println(" s");

  display.print("I: ");
  display.print(currentA_, 4);
  display.println(" A");

  display.println();
  display.println(message_);

  display.display();
}