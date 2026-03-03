#include "sensor.h"
#include <Arduino.h>

#define SENSOR_PIN A0
static const float ADC_MAX = 4095.0f;   // 12-bit
static const float VREF    = 3.3f;      // ADC reference

// "ACS725 10A V1.0" Module (Grove 0–10A): Typical sensitivity 264 mV/A
static const float SENS_MV_PER_A = 264.0f;  // 

#define FILTER_N 10
static float buffer[FILTER_N];
static int idx = 0;

static float v_offset = 0.0f; // Vout to 0A (calibrated)

static float read_voltage() {
  int adc = analogRead(SENSOR_PIN);
  return (adc / ADC_MAX) * VREF;
}

void sensor_init() {
  analogReadResolution(12);

  for (int i = 0; i < FILTER_N; i++) buffer[i] = 0.0f;
  idx = 0;

  // Calibrate offset with 0A for ~300ms
  // (motor stopped / no current through the sensor)
  const int N = 200;
  float sum = 0.0f;
  for (int i = 0; i < N; i++) {
    sum += read_voltage();
    delay(2);
  }
  v_offset = sum / N;
}

float sensor_read_current() {
  float v = read_voltage();
  float v_diff = v - v_offset;                        // V refered to 0A
  float current_A = (v_diff * 1000.0f) / SENS_MV_PER_A; // A

  // Unidirectional: avoids negative noise
  if (current_A < 0) current_A = 0;

  buffer[idx] = current_A;
  idx = (idx + 1) % FILTER_N;

  float sum = 0.0f;
  for (int i = 0; i < FILTER_N; i++) sum += buffer[i];
  return sum / FILTER_N;
}