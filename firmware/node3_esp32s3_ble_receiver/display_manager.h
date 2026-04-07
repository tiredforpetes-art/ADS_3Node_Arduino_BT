#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>

class DisplayManager {
public:
  bool begin();
  bool isAvailable() const { return available_; }

  void setBleStatus(bool connected);
  void updateFrame(uint32_t timestampMs, float currentA);
  void setFramesOk(uint32_t frames);
  void setMessage(const char* msg);

  void refresh();

private:
  bool available_ = false;
  bool bleConnected_ = false;
  uint32_t framesOk_ = 0;
  uint32_t timestampMs_ = 0;
  float currentA_ = 0.0f;
  String message_ = "Init";
};

#endif