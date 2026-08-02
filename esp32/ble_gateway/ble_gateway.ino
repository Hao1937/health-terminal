#include <Arduino.h>
#include <BLE2902.h>
#include <BLECharacteristic.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

namespace {
constexpr char kDeviceName[] = "HealthTerminal-Gateway";
constexpr char kServiceUuid[] = "0000ffe0-0000-1000-8000-00805f9b34fb";
constexpr char kCharacteristicUuid[] = "0000ffe1-0000-1000-8000-00805f9b34fb";

constexpr int kUartRxPin = 16;
constexpr int kUartTxPin = 17;
constexpr uint32_t kUartBaud = 9600;
constexpr size_t kUartBufferSize = 512;
constexpr size_t kNotifyChunkSize = 20;
constexpr uint32_t kUartIdleUs = 5000;
constexpr uint32_t kNotifyIntervalMs = 5;
constexpr uint32_t kAdvertisingRestartMs = 500;

// Set to 1 only while diagnosing the board over the USB serial port.
#define GATEWAY_DEBUG 0

HardwareSerial stm32Uart(2);
BLEServer *server = nullptr;
BLECharacteristic *notifyCharacteristic = nullptr;

volatile bool connected = false;
volatile bool notificationsEnabled = false;
volatile bool disconnectPending = false;
volatile bool subscriptionChanged = false;

uint8_t fifo[kUartBufferSize];
size_t fifoHead = 0;
size_t fifoTail = 0;
size_t fifoCount = 0;
uint32_t lastUartByteUs = 0;
uint32_t lastNotifyMs = 0;
uint32_t disconnectAtMs = 0;
bool droppingUntilIdle = false;

#if GATEWAY_DEBUG
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINTLN(x) ((void)0)
#endif

void clearFifo() {
  fifoHead = 0;
  fifoTail = 0;
  fifoCount = 0;
  lastUartByteUs = 0;
}

void discardUartUntilIdle() {
  while (stm32Uart.available() > 0) {
    stm32Uart.read();
    lastUartByteUs = micros();
  }
  if (lastUartByteUs != 0 &&
      static_cast<uint32_t>(micros() - lastUartByteUs) >= kUartIdleUs) {
    droppingUntilIdle = false;
    lastUartByteUs = 0;
  }
}

bool pushByte(uint8_t value) {
  if (fifoCount >= kUartBufferSize) {
    clearFifo();
    droppingUntilIdle = true;
    DEBUG_PRINTLN("UART FIFO overflow; dropping until idle");
    return false;
  }
  fifo[fifoHead] = value;
  fifoHead = (fifoHead + 1) % kUartBufferSize;
  ++fifoCount;
  return true;
}

size_t popBytes(uint8_t *out, size_t capacity) {
  const size_t length = fifoCount < capacity ? fifoCount : capacity;
  for (size_t i = 0; i < length; ++i) {
    out[i] = fifo[fifoTail];
    fifoTail = (fifoTail + 1) % kUartBufferSize;
  }
  fifoCount -= length;
  return length;
}

class ServerCallbacks final : public BLEServerCallbacks {
 public:
  void onConnect(BLEServer *) override {
    connected = true;
    notificationsEnabled = false;
    disconnectPending = false;
    clearFifo();
    DEBUG_PRINTLN("BLE connected");
  }

  void onDisconnect(BLEServer *) override {
    connected = false;
    notificationsEnabled = false;
    disconnectPending = true;
    disconnectAtMs = millis();
    clearFifo();
    DEBUG_PRINTLN("BLE disconnected");
  }
};

class DescriptorCallbacks final : public BLEDescriptorCallbacks {
 public:
  void onWrite(BLEDescriptor *descriptor) override {
    BLE2902 *cccd = static_cast<BLE2902 *>(descriptor);
    const bool enabled = cccd->getNotifications();
    notificationsEnabled = enabled;
    subscriptionChanged = true;
    DEBUG_PRINTLN(enabled ? "BLE notifications enabled"
                          : "BLE notifications disabled");
  }
};

void drainUart() {
  while (stm32Uart.available() > 0) {
    const int value = stm32Uart.read();
    if (value < 0) {
      break;
    }
    lastUartByteUs = micros();
    if (droppingUntilIdle) {
      continue;
    }
    if (!connected || !notificationsEnabled || !pushByte(static_cast<uint8_t>(value))) {
      if (!connected || !notificationsEnabled) {
        clearFifo();
      }
    }
  }
}

void notifyAvailable() {
  if (!connected || !notificationsEnabled || fifoCount == 0) {
    return;
  }

  const uint32_t now = millis();
  if (static_cast<uint32_t>(now - lastNotifyMs) < kNotifyIntervalMs) {
    return;
  }

  const bool fullChunk = fifoCount >= kNotifyChunkSize;
  const bool uartIdle =
      lastUartByteUs != 0 &&
      static_cast<uint32_t>(micros() - lastUartByteUs) >= kUartIdleUs;
  if (!fullChunk && !uartIdle) {
    return;
  }

  uint8_t chunk[kNotifyChunkSize];
  const size_t length = popBytes(chunk, sizeof(chunk));
  notifyCharacteristic->setValue(chunk, length);
  notifyCharacteristic->notify();
  lastNotifyMs = now;

  if (fifoCount == 0) {
    lastUartByteUs = 0;
  }
}

void restartAdvertisingIfNeeded() {
  if (!disconnectPending ||
      static_cast<uint32_t>(millis() - disconnectAtMs) < kAdvertisingRestartMs) {
    return;
  }
  disconnectPending = false;
  server->startAdvertising();
  DEBUG_PRINTLN("BLE advertising restarted");
}

void setupBle() {
  BLEDevice::init(kDeviceName);
  server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  BLEService *service = server->createService(kServiceUuid);
  notifyCharacteristic = service->createCharacteristic(
      kCharacteristicUuid, BLECharacteristic::PROPERTY_NOTIFY);
  BLE2902 *cccd = new BLE2902();
  cccd->setCallbacks(new DescriptorCallbacks());
  notifyCharacteristic->addDescriptor(cccd);
  service->start();

  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(kServiceUuid);
  advertising->setScanResponse(true);
  advertising->start();
}
}  // namespace

void setup() {
#if GATEWAY_DEBUG
  Serial.begin(115200);
#endif
  stm32Uart.setRxBufferSize(kUartBufferSize);
  stm32Uart.begin(kUartBaud, SERIAL_8N1, kUartRxPin, kUartTxPin);
  setupBle();
}

void loop() {
  if (subscriptionChanged) {
    subscriptionChanged = false;
    clearFifo();
  }

  if (!connected || !notificationsEnabled || droppingUntilIdle) {
    if (droppingUntilIdle) {
      discardUartUntilIdle();
    } else {
      while (stm32Uart.available() > 0) {
        stm32Uart.read();
      }
      clearFifo();
    }
  } else {
    drainUart();
    notifyAvailable();
  }

  restartAdvertisingIfNeeded();
  delay(1);
}
