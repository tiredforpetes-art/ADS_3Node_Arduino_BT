#include <Arduino.h>
#include <NimBLEDevice.h>

// ================= FEATURE FLAGS =================
#define USE_DISPLAY 1
#define USE_SD_LOGGER 1
#define DEBUG_SERIAL 1

#if USE_DISPLAY
  #include "display_manager.h"
#endif

#if USE_SD_LOGGER
  #include "logger.h"
#endif

// ================= BLE CONFIG =================
static const char* SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
static const char* CHAR_UUID    = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";

static const size_t FRAME_LEN = 11;
static const uint8_t START_BYTE = 0xAA;
static const uint8_t TYPE_CURRENT = 0x01;

#ifndef SD_CS_PIN
  #define SD_CS_PIN D2
#endif

// Queue for frames received from BLE notification callback.
struct RxFrame {
  uint8_t data[FRAME_LEN];
};

static constexpr uint16_t RX_QUEUE_SIZE = 128;
static RxFrame rxQueue[RX_QUEUE_SIZE];
static volatile uint16_t rxHead = 0;
static volatile uint16_t rxTail = 0;
static portMUX_TYPE rxMux = portMUX_INITIALIZER_UNLOCKED;

static NimBLEAdvertisedDevice* foundDev = nullptr;
static NimBLEClient* client = nullptr;
static NimBLERemoteCharacteristic* rChar = nullptr;

static bool connected = false;
static bool doConnect = false;

static uint32_t framesQueued = 0;
static uint32_t framesParsed = 0;
static uint32_t framesDroppedQueue = 0;
static uint32_t framesBad = 0;
static uint32_t framesLogged = 0;
static uint32_t lastTsMs = 0;
static float lastCurrentA = 0.0f;
static uint32_t lastNotifyMs = 0;

#if USE_DISPLAY
DisplayManager displayMgr;
#endif

#if USE_SD_LOGGER
DataLogger logger;
#endif

static bool enqueueFrame(const uint8_t* data, size_t len) {
  if (len != FRAME_LEN) return false;

  bool ok = false;
  portENTER_CRITICAL(&rxMux);
  uint16_t next = (uint16_t)((rxHead + 1u) % RX_QUEUE_SIZE);
  if (next != rxTail) {
    memcpy(rxQueue[rxHead].data, data, FRAME_LEN);
    rxHead = next;
    ok = true;
  }
  portEXIT_CRITICAL(&rxMux);

  if (ok) {
    framesQueued++;
  } else {
    framesDroppedQueue++;
  }

  return ok;
}

static bool dequeueFrame(RxFrame& out) {
  bool ok = false;
  portENTER_CRITICAL(&rxMux);
  if (rxTail != rxHead) {
    memcpy(out.data, rxQueue[rxTail].data, FRAME_LEN);
    rxTail = (uint16_t)((rxTail + 1u) % RX_QUEUE_SIZE);
    ok = true;
  }
  portEXIT_CRITICAL(&rxMux);
  return ok;
}

static uint16_t queuedCount() {
  uint16_t count;
  portENTER_CRITICAL(&rxMux);
  count = (rxHead >= rxTail) ? (rxHead - rxTail) : (RX_QUEUE_SIZE - rxTail + rxHead);
  portEXIT_CRITICAL(&rxMux);
  return count;
}

static bool decodeFrame(const uint8_t* data, uint32_t& ts, float& cur) {
  if (data[0] != START_BYTE) return false;
  if (data[1] != TYPE_CURRENT) return false;

  memcpy(&ts, data + 2, 4);
  memcpy(&cur, data + 6, 4);

  if (!isfinite(cur)) return false;
  return true;
}

// ================= Notify callback =================
static void onNotify(NimBLERemoteCharacteristic* c, uint8_t* data, size_t len, bool isNotify) {
  (void)c;
  (void)isNotify;

  if (len != FRAME_LEN) {
    framesBad++;
    return;
  }

  if (data[0] != START_BYTE) {
    framesBad++;
    return;
  }

  lastNotifyMs = millis();
  enqueueFrame(data, len);
}

// ================= Client callbacks =================
class ClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient* pClient) override {
    (void)pClient;
#if DEBUG_SERIAL
    Serial.println("Connected to Node 2");
#endif
    connected = true;

#if USE_DISPLAY
    displayMgr.setBleStatus(true);
    displayMgr.setMessage("Connected");
#endif
  }

  void onDisconnect(NimBLEClient* pClient, int reason) override {
    (void)pClient;
    (void)reason;
#if DEBUG_SERIAL
    Serial.println("Disconnected from Node 2");
#endif
    connected = false;
    doConnect = false;
    rChar = nullptr;

    if (foundDev) {
      delete foundDev;
      foundDev = nullptr;
    }

#if USE_DISPLAY
    displayMgr.setBleStatus(false);
    displayMgr.setMessage("Disconnected");
#endif

    NimBLEDevice::getScan()->start(0, false, false);
  }
};

// ================= Scan callbacks =================
class ScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* adv) override {
#if DEBUG_SERIAL
    Serial.print("Found device: ");
    Serial.println(adv->toString().c_str());
#endif

    if (adv->isAdvertisingService(NimBLEUUID(SERVICE_UUID))) {
#if DEBUG_SERIAL
      Serial.println("Target service found");
#endif

#if USE_DISPLAY
      displayMgr.setMessage("Node2 found");
#endif

      if (foundDev) {
        delete foundDev;
        foundDev = nullptr;
      }

      foundDev = new NimBLEAdvertisedDevice(*adv);
      doConnect = true;
      NimBLEDevice::getScan()->stop();
    }
  }
};

// ================= Connect and subscribe =================
static bool connectAndSubscribe() {
  if (!foundDev) return false;

#if DEBUG_SERIAL
  Serial.println("Connecting to Node 2...");
#endif

#if USE_DISPLAY
  displayMgr.setMessage("Connecting...");
#endif

  client = NimBLEDevice::createClient();
  client->setClientCallbacks(new ClientCallbacks(), false);

  if (!client->connect(foundDev)) {
#if DEBUG_SERIAL
    Serial.println("Connection failed");
#endif
#if USE_DISPLAY
    displayMgr.setMessage("Conn failed");
#endif
    NimBLEDevice::deleteClient(client);
    client = nullptr;
    return false;
  }

  NimBLERemoteService* svc = client->getService(SERVICE_UUID);
  if (!svc) {
#if DEBUG_SERIAL
    Serial.println("Service not found");
#endif
#if USE_DISPLAY
    displayMgr.setMessage("No service");
#endif
    client->disconnect();
    return false;
  }

  rChar = svc->getCharacteristic(CHAR_UUID);
  if (!rChar) {
#if DEBUG_SERIAL
    Serial.println("Characteristic not found");
#endif
#if USE_DISPLAY
    displayMgr.setMessage("No char");
#endif
    client->disconnect();
    return false;
  }

  if (!rChar->canNotify()) {
#if DEBUG_SERIAL
    Serial.println("Characteristic cannot notify");
#endif
#if USE_DISPLAY
    displayMgr.setMessage("No notify");
#endif
    client->disconnect();
    return false;
  }

  if (!rChar->subscribe(true, onNotify)) {
#if DEBUG_SERIAL
    Serial.println("Subscribe failed");
#endif
#if USE_DISPLAY
    displayMgr.setMessage("Sub failed");
#endif
    client->disconnect();
    return false;
  }

#if DEBUG_SERIAL
  Serial.println("Subscribed. Waiting for notifications...");
#endif

#if USE_DISPLAY
  displayMgr.setMessage("Subscribed");
#endif

  return true;
}

static void processPendingFrames() {
  RxFrame frame;

  while (dequeueFrame(frame)) {
    uint32_t ts = 0;
    float cur = 0.0f;

    if (!decodeFrame(frame.data, ts, cur)) {
      framesBad++;
      continue;
    }

    framesParsed++;
    lastTsMs = ts;
    lastCurrentA = cur;

#if DEBUG_SERIAL
    Serial.print("BLE RX t=");
    Serial.print(ts / 1000.0f, 3);
    Serial.print(" s ; I=");
    Serial.print(cur, 4);
    Serial.println(" A");
#endif

#if USE_DISPLAY
    displayMgr.updateFrame(ts, cur);
    displayMgr.setFramesOk(framesParsed);
    displayMgr.setMessage("Receiving");
#endif

#if USE_SD_LOGGER
    if (logger.logFrame(ts, cur)) {
      framesLogged++;
    }
#endif
  }
}

void setup() {
#if DEBUG_SERIAL
  Serial.begin(115200);
  delay(200);
  Serial.println("Node 3 - BLE Client PRO");
#endif

#if USE_DISPLAY
  if (displayMgr.begin()) {
#if DEBUG_SERIAL
    Serial.println("OLED detected");
#endif
    displayMgr.setMessage("Init OK");
  } else {
#if DEBUG_SERIAL
    Serial.println("No OLED detected");
#endif
  }
#endif

#if USE_SD_LOGGER
#if DEBUG_SERIAL
  Serial.println("---- SD INIT TEST ----");
#endif
  if (logger.begin(SD_CS_PIN, "node3_log")) {
    Serial.print("SD logging to: ");
    Serial.println(logger.filePath());
  } else {
    Serial.println("SD FAIL");
  }
#endif

  NimBLEDevice::init("NODE3_RECEIVER");

  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setScanCallbacks(new ScanCallbacks(), false);
  scan->setActiveScan(true);

#if DEBUG_SERIAL
  Serial.println("Scanning for Node 2...");
#endif

#if USE_DISPLAY
  displayMgr.setMessage("Scanning...");
#endif

  scan->start(0, false, false);
}

void loop() {
  if (doConnect && !connected) {
    doConnect = false;

    if (!connectAndSubscribe()) {
#if DEBUG_SERIAL
      Serial.println("Connect/subscribe failed. Restarting scan...");
#endif

#if USE_DISPLAY
      displayMgr.setMessage("Retry scan");
#endif

      if (foundDev) {
        delete foundDev;
        foundDev = nullptr;
      }
      NimBLEDevice::getScan()->start(0, false, false);
    }
  }

  processPendingFrames();

#if USE_DISPLAY
  displayMgr.refresh();
#endif

#if USE_SD_LOGGER
  static uint32_t lastPeriodicFlush = 0;
  if (millis() - lastPeriodicFlush >= 1000u) {
    logger.flush();
    lastPeriodicFlush = millis();
  }
#endif

#if DEBUG_SERIAL
  static uint32_t lastDbg = 0;
  if (millis() - lastDbg >= 1000u) {
    lastDbg = millis();
    Serial.print("[DBG] queued=");
    Serial.print(framesQueued);
    Serial.print(" parsed=");
    Serial.print(framesParsed);
    Serial.print(" logged=");
    Serial.print(framesLogged);
    Serial.print(" dropQ=");
    Serial.print(framesDroppedQueue);
    Serial.print(" bad=");
    Serial.print(framesBad);
    Serial.print(" qNow=");
    Serial.println(queuedCount());
  }
#endif

  delay(10);
}
