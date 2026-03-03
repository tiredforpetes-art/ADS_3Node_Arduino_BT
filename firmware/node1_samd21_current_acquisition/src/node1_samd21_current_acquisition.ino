/************************************************************
 *Node 1 - Current Acquisition Node
* Functionality (unchanged):

* - Reads current via ADC (A0) with moving average filter
* - If a sensor is connected (abs(current) > IDLE_THRESHOLD):
* Sends binary frame via UART (Serial1)
* Prints debug data via USB (Serial/SerialUSB)
* - Calculates averages: samples every SAMPLE_INTERVAL_MS and saves one average value every LOG_INTERVAL_MS to a microSD card
* + (Improved) Saves cumulative V, P(W), and E(Wh), and optionally the peak value per window
 ************************************************************/

#include "sensor.h"
#include "display.h"
#include "protocol.h"
#include "logger.h"

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <math.h>

// ================= UART =================
#define UART_BAUD 115200

#if defined(ARDUINO_ARCH_ESP32)
  #define UART_TX 4
  #define UART_RX 5
  static inline void uart_begin() {
    Serial1.begin(UART_BAUD, SERIAL_8N1, UART_RX, UART_TX);
  }
#else
  static inline void uart_begin() {
    Serial1.begin(UART_BAUD);
  }
#endif

// ================= SD =================
#ifndef SD_CS_PIN
  #define SD_CS_PIN 2
#endif

// ====== LOG CONFIG (editable) ======
static uint32_t SAMPLE_INTERVAL_MS = 500;   // sampling for measurement
static uint32_t LOG_INTERVAL_MS    = 1000;  // 1-row CSV file saved

// ====== SUPPLY CONFIG (editable) ======
static float SUPPLY_VOLTAGE_V = 31.0f;      // fixed source voltage (if not measured)

// ====== DEBUG ======
#define DEBUG_DISPLAY 1
#define DEBUG_SERIAL  1
#define IDLE_THRESHOLD 0.002
#define DEBUG_FRAME   0

uint8_t frame[16];
uint32_t frameCounter = 0;

DataLogger logger;

// ---- USB Serial helper ----
static const uint32_t USB_WAIT_MS = 1500;

static inline void usb_serial_begin() {
#if defined(ARDUINO_ARCH_SAMD) && defined(SerialUSB)
  SerialUSB.begin(115200);
  uint32_t t0 = millis();
  while (!SerialUSB && (millis() - t0 < USB_WAIT_MS)) { delay(10); }
#else
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && (millis() - t0 < USB_WAIT_MS)) { delay(10); }
#endif
}

static inline Stream& usb_serial() {
#if defined(ARDUINO_ARCH_SAMD) && defined(SerialUSB)
  return SerialUSB;
#else
  return Serial;
#endif
}

static inline bool usb_ready() {
#if defined(ARDUINO_ARCH_SAMD) && defined(SerialUSB)
  return (bool)SerialUSB;
#else
  return (bool)Serial;
#endif
}

void setup() {
  usb_serial_begin();
  uart_begin();

  sensor_init();
  display_init();

  // SD logger init
  if (!logger.begin(SD_CS_PIN, "/log")) {
    if (usb_ready()) usb_serial().println("[SD] ERROR: mount failed");
  } else {
    logger.setSampleIntervalMs(SAMPLE_INTERVAL_MS);
    logger.setLogIntervalMs(LOG_INTERVAL_MS);

    // ====== RECOMENDATIONS (LOGGER UPDATE) ======
    logger.setSupplyVoltage(SUPPLY_VOLTAGE_V);  // to calculate P(W)=V*I y E(Wh)
    logger.setLogPeak(true);                    // save Peak_A in window

    if (usb_ready()) {
      usb_serial().print("[SD] Logging to: ");
      usb_serial().println(logger.filePath());
    }
  }

  if (usb_ready()) usb_serial().println("Node 1 - Current Acquisition Ready");
}

void loop() {
  uint32_t timestamp = millis();
  float current = sensor_read_current();

  bool sensor_connected = fabs(current) > IDLE_THRESHOLD;

  if (sensor_connected) {
    size_t frame_size = buildCurrentFrame(frame, timestamp, current);
    Serial1.write(frame, frame_size);
    frameCounter++;

#if DEBUG_SERIAL
    if (usb_ready()) {
      usb_serial().print("DATA -> Time: ");
      usb_serial().print(timestamp / 1000.0, 3);
      usb_serial().print(" s ; Current: ");
      usb_serial().print(current, 4);
      usb_serial().println(" A");
    }
#endif

#if DEBUG_FRAME
    if (usb_ready()) {
      usb_serial().print("FRAME -> ");
      for (size_t i = 0; i < frame_size; i++) {
        if (frame[i] < 16) usb_serial().print("0");
        usb_serial().print(frame[i], HEX);
        usb_serial().print(" ");
      }
      usb_serial().println();
    }
#endif
  }

  // Logging (average/peak per window; CSV includes V, P(W) and E(Wh))
  logger.update(current, sensor_connected);

#if DEBUG_DISPLAY
  display_status(current, timestamp, frameCounter, sensor_connected);
#endif

  delay(200);
}