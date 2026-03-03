#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <NimBLEDevice.h>

// ================= UART (from Node 1) =================
static const uint32_t UART_BAUD = 115200;
static const int UART_RX = D7;   // Expansion UART: RX-D7
static const int UART_TX = D6;   // Expansion UART: TX-D6 (Not necessary if only receive)

// ================= PROTO FRAME V2 =================
// [0]=0xAA [1]=TYPE [2..5]=timestamp(u32 LE) [6..9]=current(float LE) [10]=CRC8(frame[1..9])
static const uint8_t START_BYTE = 0xAA;
static const uint8_t TYPE_CURRENT = 0x01;
static const size_t FRAME_LEN = 11;

// ================= OLED =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
static const uint8_t OLED_ADDR = 0x3C;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ================= BLE =================
// UUIDs (can change them if want; keep them the same in Node 3)
static const char* BLE_DEVICE_NAME = "NODE2_BRIDGE";
static const char* SERVICE_UUID    = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
static const char* CHAR_UUID       = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"; // Notify

static NimBLECharacteristic* pChar = nullptr;
static volatile bool bleHasClient = false;

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* s, NimBLEConnInfo& connInfo) override {
    bleHasClient = true;
  }
  void onDisconnect(NimBLEServer* s, NimBLEConnInfo& connInfo, int reason) override {
    bleHasClient = false;
    NimBLEDevice::startAdvertising();
  }
};

// ================= CRC8 (same as Node 1) =================
uint8_t crc8(const uint8_t *data, size_t len) {
  uint8_t crc = 0x00;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 0x80) crc = (crc << 1) ^ 0x07;
      else crc <<= 1;
    }
  }
  return crc;
}

// ================= Frame parser =================
static uint8_t buf[FRAME_LEN];
static size_t idx = 0;
static bool syncing = true;

static uint32_t framesOk = 0;
static uint32_t framesCrcErr = 0;
static uint32_t framesOther = 0;

static float lastCurrentA = 0.0f;
static uint32_t lastTsMs = 0;

static void resetParser() {
  idx = 0;
  syncing = true;
}

static void forwardBleRawFrame(const uint8_t* frame) {
  if (!pChar || !bleHasClient) return;
  pChar->setValue(frame, FRAME_LEN);
  pChar->notify();
}

static void handleFrame(const uint8_t* f) {
  // Decode
  memcpy(&lastTsMs, f + 2, 4);
  memcpy(&lastCurrentA, f + 6, 4);

  framesOk++;

  // Forward via BLE (raw 11-byte frame)
  forwardBleRawFrame(f);
}

static void feedByte(uint8_t b) {
  if (syncing) {
    if (b == START_BYTE) {
      buf[0] = b;
      idx = 1;
      syncing = false;
    }
    return;
  }

  buf[idx++] = b;
  if (idx < FRAME_LEN) return;

  // Validación
  if (buf[0] != START_BYTE) {
    framesOther++;
    resetParser();
    return;
  }

  const uint8_t crc_calc = crc8(buf + 1, 9);
  const uint8_t crc_rx   = buf[10];

  if (crc_calc != crc_rx) {
    framesCrcErr++;

    // Resync looking for START within the buffer
    int newStart = -1;
    for (int i = 1; i < (int)FRAME_LEN; i++) {
      if (buf[i] == START_BYTE) { newStart = i; break; }
    }
    if (newStart >= 0) {
      const int remaining = (int)FRAME_LEN - newStart;
      memmove(buf, buf + newStart, remaining);
      idx = remaining;
      syncing = false;
      return;
    }
    resetParser();
    return;
  }

  if (buf[1] == TYPE_CURRENT) {
    handleFrame(buf);
  } else {
    framesOther++;
  }

  resetParser();
}

// ================= OLED update =================
static uint32_t lastOledMs = 0;

static void oledInit() {
  Wire.begin(D4, D5); // SDA=D4, SCL=D5 (Expansion Board)
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    // If there's no OLED, we still can't block it.
    return;
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Node2 UART->BLE");
  display.display();
}

static void oledUpdate() {
  if (!display.width()) return; // if not initialized
  display.clearDisplay();
  display.setCursor(0, 0);

  display.println("NODE 2 BRIDGE");
  display.print("Frames OK: "); display.println(framesOk);
  display.print("CRC Err : ");  display.println(framesCrcErr);
  display.print("BLE conn: ");  display.println(bleHasClient ? "YES" : "NO");

  display.println();
  display.print("I(A): "); display.println(lastCurrentA, 4);
  display.print("t(s): "); display.println(lastTsMs / 1000.0, 3);

  display.display();
}

// ================= BLE init =================
static void bleInit() {
  NimBLEDevice::init(BLE_DEVICE_NAME);

  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  NimBLEService* service = server->createService(SERVICE_UUID);
  pChar = service->createCharacteristic(
    CHAR_UUID,
    NIMBLE_PROPERTY::NOTIFY
  );

  service->start();

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->start();
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("Node 2 - UART Receiver + OLED + BLE Notify");

  // UART
  Serial1.begin(UART_BAUD, SERIAL_8N1, UART_RX, UART_TX);
  while (Serial1.available()) Serial1.read();

  oledInit();
  bleInit();
}

void loop() {
  while (Serial1.available() > 0) {
    int c = Serial1.read();
    if (c >= 0) feedByte((uint8_t)c);
  }

  const uint32_t now = millis();
  if (now - lastOledMs >= 200) { // refresco OLED suave
    lastOledMs = now;
    oledUpdate();
  }

  delay(2);
}
