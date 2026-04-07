#include "logger.h"
#include <SPI.h>
#include <Arduino.h>

static void printFixed(File& f, double v, int decimals) {
  char buf[32];
  dtostrf(v, 0, decimals, buf);
  f.print(buf);
}

bool DataLogger::pickNextFilename(const char* baseName) {
  for (int i = 0; i < 1000; i++) {
    snprintf(filename_, sizeof(filename_), "/%s_%03d.csv", baseName, i);
    if (!SD.exists(filename_)) {
      Serial.print("[LOGGER] New file selected: ");
      Serial.println(filename_);
      return true;
    }
  }

  snprintf(filename_, sizeof(filename_), "/%s_full.csv", baseName);
  Serial.print("[LOGGER] Fallback file selected: ");
  Serial.println(filename_);
  return true;
}

bool DataLogger::writeHeader() {
  File f = SD.open(filename_, FILE_WRITE);
  if (!f) {
    Serial.println("[LOGGER] ERROR: could not create file");
    return false;
  }

  f.println("Time_s ; Current_A ; Unit");
  f.flush();
  f.close();
  Serial.println("[LOGGER] Header written");
  return true;
}

bool DataLogger::openForAppend() {
  if (file_) {
    return true;
  }

#ifdef FILE_APPEND
  file_ = SD.open(filename_, FILE_APPEND);
#else
  file_ = SD.open(filename_, FILE_WRITE);
  if (file_) file_.seek(file_.size());
#endif

  if (!file_) {
    Serial.println("[LOGGER] ERROR: could not open log file in append mode");
    return false;
  }

#ifndef FILE_APPEND
  file_.seek(file_.size());
#endif

  return true;
}

bool DataLogger::appendLine(double t_s, double currentA) {
  if (!openForAppend()) {
    writeErrors_++;
    return false;
  }

  printFixed(file_, t_s, 3);
  file_.print(" ; ");
  printFixed(file_, currentA, 4);
  file_.print(" ; A\r\n");

  if (!file_) {
    Serial.println("[LOGGER] ERROR: write failed");
    writeErrors_++;
    file_.close();
    return false;
  }

  linesWritten_++;

  if ((linesWritten_ % 10u) == 0u || (millis() - lastFlushMs_ >= 1000u)) {
    file_.flush();
    lastFlushMs_ = millis();
  }

  Serial.print("[LOGGER] Logged: ");
  Serial.print(t_s, 3);
  Serial.print(" s ; ");
  Serial.print(currentA, 4);
  Serial.println(" A");

  return true;
}

bool DataLogger::begin(uint8_t csPin, const char* baseName) {
  Serial.print("[LOGGER] Initializing SD on CS pin ");
  Serial.println(csPin);

  pinMode(csPin, OUTPUT);
  SPI.begin(D8, D9, D10, csPin);

  sdOk_ = SD.begin(csPin, SPI);
  if (!sdOk_) {
    Serial.println("[LOGGER] ERROR: SD.begin failed");
    return false;
  }

  Serial.println("[LOGGER] SD.begin OK");

  pickNextFilename(baseName);

  if (!SD.exists(filename_)) {
    Serial.print("[LOGGER] Creating file: ");
    Serial.println(filename_);
    if (!writeHeader()) {
      sdOk_ = false;
      return false;
    }
  }

  if (!openForAppend()) {
    sdOk_ = false;
    return false;
  }

  lastFlushMs_ = millis();
  return true;
}

bool DataLogger::logFrame(uint32_t timestampMs, float currentA) {
  if (!sdOk_) {
    Serial.println("[LOGGER] SD not initialized, skipping log");
    return false;
  }

  return appendLine(timestampMs / 1000.0, currentA);
}

bool DataLogger::flush() {
  if (!sdOk_ || !file_) return false;
  file_.flush();
  lastFlushMs_ = millis();
  return true;
}

void DataLogger::end() {
  if (file_) {
    file_.flush();
    file_.close();
  }
}
