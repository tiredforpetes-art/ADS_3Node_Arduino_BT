#include <Arduino.h>
#include <NimBLEDevice.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ================= UART =================
static const uint32_t UART_BAUD = 115200;
static const int UART_RX = D7;
static const int UART_TX = D6;

// ================= PROTOCOL (Frame V2) =================
static const uint8_t START_BYTE   = 0xAA;
static const uint8_t TYPE_CURRENT = 0x01;
static const size_t  FRAME_LEN    = 11;

// ================= BLE =================
static const char* BLE_DEVICE_NAME = "NODE2_BRIDGE";
static const char* SERVICE_UUID    = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
static const char* CHAR_UUID       = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";

static NimBLECharacteristic* pChar = nullptr;
static volatile bool bleHasClient = false;

// ================= OLED (optional) =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
bool oledPresent = false;

// ================= Counters =================
static uint32_t framesOk = 0;
static uint32_t framesCrcErr = 0;

static uint32_t lastTsMs = 0;
static float lastCurrentA = 0.0f;
static uint32_t lastFrameRxMs = 0;

static uint32_t framesOk_window = 0;
static uint32_t bytes_window = 0;

// ================= CRC8 =================
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

// ================= BLE =================
class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer*, NimBLEConnInfo&) override {
    bleHasClient = true;
  }
  void onDisconnect(NimBLEServer*, NimBLEConnInfo&, int) override {
    bleHasClient = false;
    NimBLEDevice::startAdvertising();
  }
};

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

  Serial.println("BLE advertising started");
}

// ================= OLED INIT =================
void oledInit() {

  Wire.begin(D4, D5);

  if (display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {

    oledPresent = true;

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0,0);
    display.println("NODE2 UART->BLE");
    display.println("Display OK");
    display.display();

    Serial.println("OLED detected");

  } else {

    Serial.println("No OLED detected");
  }
}

// ================= OLED STATUS =================
void displayStatus() {

  if (!oledPresent) return;

  display.clearDisplay();

  display.setCursor(0,0);
  display.println("NODE2 UART->BLE");

  display.print("Frames:");
  display.println(framesOk);

  display.print("CRC:");
  display.println(framesCrcErr);

  display.print("BLE:");
  display.println(bleHasClient ? "ON" : "OFF");

  display.print("I:");
  display.print(lastCurrentA,3);
  display.println("A");

  display.display();
}

// ================= UART parser =================
static uint8_t buf[FRAME_LEN];
static size_t idx = 0;
static bool syncing = true;

static inline void resetParser() {
  idx = 0;
  syncing = true;
}

static inline void forwardBleRawFrame(const uint8_t* frame) {
  if (!pChar || !bleHasClient) return;
  pChar->setValue(frame, FRAME_LEN);
  pChar->notify();
}

static void handleFrame(const uint8_t* f) {

  if (f[1] != TYPE_CURRENT) return;

  memcpy(&lastTsMs, f + 2, 4);
  memcpy(&lastCurrentA, f + 6, 4);

  lastFrameRxMs = millis();

  framesOk++;
  framesOk_window++;

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

  buf[idx++]=b;

  if (idx < FRAME_LEN) return;

  const uint8_t crc_calc = crc8(buf+1,9);
  const uint8_t crc_rx = buf[10];

  if (crc_calc != crc_rx) {

    framesCrcErr++;

    resetParser();
    return;
  }

  handleFrame(buf);
  resetParser();
}

// ================= SERIAL DEBUG =================
static uint32_t lastPrintMs = 0;

static void printStatusPro() {

  const uint32_t now = millis();
  const uint32_t ageMs = (lastFrameRxMs==0)?0:(now-lastFrameRxMs);

  Serial.print("FramesOK=");
  Serial.print(framesOk);

  Serial.print("  CRCerr=");
  Serial.print(framesCrcErr);

  Serial.print("  BLE=");
  Serial.print(bleHasClient ? "ON":"OFF");

  Serial.print("  rate=");
  Serial.print(framesOk_window);
  Serial.print(" frm/s");

  Serial.print("  bytes=");
  Serial.print(bytes_window);
  Serial.print(" B/s");

  Serial.print("  age=");
  Serial.print(ageMs);
  Serial.print(" ms");

  Serial.print("  t=");
  Serial.print(lastTsMs/1000.0,3);
  Serial.print("s");

  Serial.print("  I=");
  Serial.print(lastCurrentA,4);
  Serial.println("A");

  framesOk_window=0;
  bytes_window=0;
}

// ================= SETUP =================
void setup() {

  Serial.begin(115200);
  delay(200);

  Serial.println("Node 2 - UART->BLE bridge");

  Serial1.begin(UART_BAUD,SERIAL_8N1,UART_RX,UART_TX);

  while(Serial1.available()) Serial1.read();

  oledInit();

  bleInit();
}

// ================= LOOP =================
void loop() {

  while (Serial1.available()>0) {

    int c = Serial1.read();

    if (c>=0) {
      bytes_window++;
      feedByte((uint8_t)c);
    }
  }

  const uint32_t now = millis();

  if(now-lastPrintMs>=1000){

    lastPrintMs=now;

    printStatusPro();

    displayStatus();
  }

  delay(2);
}