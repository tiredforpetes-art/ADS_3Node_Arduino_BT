#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <SD.h>

class DataLogger {
public:
  // csPin: CS pin of the microSD card
  // baseDir: folder where runs are saved, for example "/log"
  bool begin(uint8_t csPin, const char* baseDir = "/log");

  // Call this in loop() with the current and the sensor state.
  void update(float current_A, bool sensorConnected);

  // Editable settings
  void setSampleIntervalMs(uint32_t ms);
  void setLogIntervalMs(uint32_t ms);

  // NEW: Supply voltage used for power/energy
  void setSupplyVoltage(float v) { supplyV = v; }

  // NEW: Enable/disable interval peak saving
  void setLogPeak(bool en) { logPeak = en; }

  // Estate
  bool isOk() const { return sd_ok; }
  const char* filePath() const { return filename; }

private:
  bool sd_ok = false;

  // Selected file (incremental)
  char filename[32] = {0};

  uint32_t sampleIntervalMs = 1000;
  uint32_t logIntervalMs    = 10000;

  uint32_t lastSampleMs = 0;
  uint32_t lastLogMs    = 0;

  double sumA = 0.0;
  uint32_t nSamples = 0;

  // NEW: peak in window
  float peakA = 0.0f;
  bool  logPeak = true;

  // NEW: accumulated energy
  float supplyV = 31.0f;   // default 31V
  double energy_Wh = 0.0;

  // Helpers
  bool ensureDir(const char* baseDir);
  bool pickNextFilename(const char* baseDir);
  void writeHeaderIfNew();
  void appendLine(double t_s, double meanA, double peakA, double V, double P, double Ewh);
};

#endif