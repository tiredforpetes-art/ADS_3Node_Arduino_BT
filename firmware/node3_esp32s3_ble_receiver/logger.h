#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <SD.h>

class DataLogger {
public:
  bool begin(uint8_t csPin, const char* baseName = "node3_log");
  void end();

  bool isOk() const { return sdOk_; }
  const char* filePath() const { return filename_; }

  bool logFrame(uint32_t timestampMs, float currentA);
  bool flush();

  uint32_t linesWritten() const { return linesWritten_; }
  uint32_t writeErrors() const { return writeErrors_; }

private:
  bool sdOk_ = false;
  char filename_[32] = {0};
  File file_;
  uint32_t linesWritten_ = 0;
  uint32_t writeErrors_ = 0;
  uint32_t lastFlushMs_ = 0;

  bool pickNextFilename(const char* baseName);
  bool openForAppend();
  bool writeHeader();
  bool appendLine(double t_s, double currentA);
};

#endif
