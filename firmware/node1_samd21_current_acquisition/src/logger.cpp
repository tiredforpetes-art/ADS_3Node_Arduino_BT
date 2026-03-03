#include "logger.h"

static void printFixed(File& f, double v, int decimals) {
  char buf[32];
  dtostrf(v, 0, decimals, buf);
  f.print(buf);
}

bool DataLogger::ensureDir(const char* baseDir) {
  // Creates directory if it doesn't exist (it's okay if it already exists)
  SD.mkdir(baseDir);
  return true;
}

bool DataLogger::pickNextFilename(const char* baseDir) {
  // Generates /log/run_000.csv ... /log/run_999.csv
  for (int i = 0; i < 1000; i++) {
    snprintf(filename, sizeof(filename), "%s/run_%03d.csv", baseDir, i);
    if (!SD.exists(filename)) {
      return true;
    }
  }
  snprintf(filename, sizeof(filename), "%s/run_full.csv", baseDir);
  return true;
}

void DataLogger::writeHeaderIfNew() {
  if (!SD.exists(filename)) {
    File f = SD.open(filename, FILE_WRITE);
    if (f) {
      // CSV consistent for Excel (delimiter ;)
      // Includes peak if logPeak==true, otherwise leave it the same but with an empty Peak_A column.
      f.println("Time_s;Mean_A;Peak_A;V_V;P_W;E_Wh");
      f.close();
    }
  }
}

void DataLogger::appendLine(double t_s, double meanA, double peakA_, double V, double P, double Ewh) {
  File f = SD.open(filename, FILE_WRITE);
  if (!f) return;

  f.seek(f.size()); // append

  printFixed(f, t_s, 3);   f.print(';');
  printFixed(f, meanA, 4); f.print(';');

  if (logPeak) printFixed(f, peakA_, 4);
  f.print(';');

  printFixed(f, V, 2);     f.print(';');
  printFixed(f, P, 3);     f.print(';');
  printFixed(f, Ewh, 6);   f.print("\r\n");

  f.close();
}

bool DataLogger::begin(uint8_t csPin, const char* baseDir) {
  sd_ok = SD.begin(csPin);
  if (!sd_ok) return false;

  ensureDir(baseDir);
  pickNextFilename(baseDir);
  writeHeaderIfNew();

  // Reset states for new session
  lastSampleMs = millis();
  lastLogMs    = millis();
  sumA = 0.0;
  nSamples = 0;
  peakA = 0.0f;
  energy_Wh = 0.0;

  return true;
}

void DataLogger::setSampleIntervalMs(uint32_t ms) { sampleIntervalMs = ms; }
void DataLogger::setLogIntervalMs(uint32_t ms)    { logIntervalMs = ms; }

void DataLogger::update(float current_A, bool sensorConnected) {
  const uint32_t now = millis();
  if (!sd_ok) return;

  if (!sensorConnected) {
    sumA = 0.0;
    nSamples = 0;
    peakA = 0.0f;
    return;
  }

  // 1) Sampling for mean
  if (now - lastSampleMs >= sampleIntervalMs) {
    lastSampleMs = now;
    sumA += (double)current_A;
    nSamples++;

    if (logPeak && current_A > peakA) peakA = current_A;
  }

  // 2) Each LOG_INTERVAL stores average + calculates power/energy
  if (now - lastLogMs >= logIntervalMs) {
    const uint32_t dt_ms = now - lastLogMs;
    lastLogMs += logIntervalMs;
    if (lastLogMs == 0) lastLogMs = now;

    if (nSamples > 0) {
      const double meanA = sumA / (double)nSamples;
      const double t_s   = now / 1000.0;

      const double V = (double)supplyV;
      const double P = V * meanA;

      // Wh = W * hours
      const double dt_h = (dt_ms / 1000.0) / 3600.0;
      energy_Wh += P * dt_h;

      appendLine(t_s, meanA, (double)peakA, V, P, energy_Wh);
    }

    sumA = 0.0;
    nSamples = 0;
    peakA = 0.0f;
  }
}