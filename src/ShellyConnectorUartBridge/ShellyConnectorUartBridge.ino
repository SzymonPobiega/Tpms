#include <WiFi.h>
#include <esp_now.h>
#include <esp_mac.h>
#include "ContractsInclude.hpp"

// -------------------- USER CONFIG --------------------
static const uint8_t broadcastMAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static const int UART_RX_PIN = 16;     // GPIO16
static const int UART_TX_PIN = 17;     // GPIO17
static const uint32_t UART_BAUD = 115200;

// UART framing on the wire:
//   [len:1][type:1][value:len-1]
// ESP-NOW payload carries only:
//   [type:1][value:len-1]
static const uint8_t MAX_LEN_FIELD = 250;     // safe/common ESP-NOW payload limit
static const size_t  MAX_UART_FRAME = 1 + MAX_LEN_FIELD;
static const size_t  MAX_ESPNOW_PAYLOAD = MAX_LEN_FIELD;

// Tiny fixed queues to keep callbacks short
static const uint8_t RX_QUEUE_SIZE = 8;   // ESP-NOW -> UART
static const uint8_t TX_QUEUE_SIZE = 8;   // UART -> ESP-NOW
// -----------------------------------------------------

static bool bridgeEnabled = false;
static unsigned long lastKeepAliveMs = 0;
static const unsigned long KEEPALIVE_TIMEOUT_MS = 5UL * 60UL * 1000UL;

HardwareSerial &U = Serial1;

// One queued packet = data bytes + length
struct Packet {
  uint16_t len;
  uint8_t  data[MAX_ESPNOW_PAYLOAD];
};

// ESP-NOW RX queue
static Packet rxQueue[RX_QUEUE_SIZE];
static volatile uint8_t rxHead = 0;
static volatile uint8_t rxTail = 0;
static volatile uint32_t rxDropped = 0;

// UART TX queue
static Packet txQueue[TX_QUEUE_SIZE];
static volatile uint8_t txHead = 0;
static volatile uint8_t txTail = 0;
static volatile uint32_t txDropped = 0;

// Send pacing
static volatile bool sendInFlight = false;
static volatile esp_now_send_status_t lastSendStatus = ESP_NOW_SEND_FAIL;
static volatile uint32_t sendOkCount = 0;
static volatile uint32_t sendFailCount = 0;

// UART read scratch buffer (contains [len][type][value...])
static uint8_t uartBuf[MAX_UART_FRAME];

static portMUX_TYPE queueMux = portMUX_INITIALIZER_UNLOCKED;

// ---------- Queue helpers ----------
static inline bool queueIsFull(uint8_t head, uint8_t tail, uint8_t size) {
  return (uint8_t)((head + 1) % size) == tail;
}

static inline bool queueIsEmpty(uint8_t head, uint8_t tail) {
  return head == tail;
}

bool pushRxPacketFromISR(const uint8_t *data, uint16_t len) {
  bool ok = false;
  portENTER_CRITICAL_ISR(&queueMux);
  if (!queueIsFull(rxHead, rxTail, RX_QUEUE_SIZE)) {
    rxQueue[rxHead].len = len;
    memcpy(rxQueue[rxHead].data, data, len);
    rxHead = (uint8_t)((rxHead + 1) % RX_QUEUE_SIZE);
    ok = true;
  } else {
    rxDropped++;
  }
  portEXIT_CRITICAL_ISR(&queueMux);
  return ok;
}

bool popRxPacket(Packet &out) {
  bool ok = false;
  portENTER_CRITICAL(&queueMux);
  if (!queueIsEmpty(rxHead, rxTail)) {
    out.len = rxQueue[rxTail].len;
    memcpy(out.data, rxQueue[rxTail].data, out.len);
    rxTail = (uint8_t)((rxTail + 1) % RX_QUEUE_SIZE);
    ok = true;
  }
  portEXIT_CRITICAL(&queueMux);
  return ok;
}

bool pushTxPacket(const uint8_t *data, uint16_t len) {
  bool ok = false;
  portENTER_CRITICAL(&queueMux);
  if (!queueIsFull(txHead, txTail, TX_QUEUE_SIZE)) {
    txQueue[txHead].len = len;
    memcpy(txQueue[txHead].data, data, len);
    txHead = (uint8_t)((txHead + 1) % TX_QUEUE_SIZE);
    ok = true;
  } else {
    txDropped++;
  }
  portEXIT_CRITICAL(&queueMux);
  return ok;
}

bool peekTxPacket(Packet &out) {
  bool ok = false;
  portENTER_CRITICAL(&queueMux);
  if (!queueIsEmpty(txHead, txTail)) {
    out.len = txQueue[txTail].len;
    memcpy(out.data, txQueue[txTail].data, out.len);
    ok = true;
  }
  portEXIT_CRITICAL(&queueMux);
  return ok;
}

bool dropTxPacket() {
  bool ok = false;
  portENTER_CRITICAL(&queueMux);
  if (!queueIsEmpty(txHead, txTail)) {
    txTail = (uint8_t)((txTail + 1) % TX_QUEUE_SIZE);
    ok = true;
  }
  portEXIT_CRITICAL(&queueMux);
  return ok;
}

// ---------- ESP-NOW callbacks ----------
void onEspNowRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (!data || len <= 0 || len > MAX_ESPNOW_PAYLOAD) return;

  uint8_t type = data[0];

  if (type == Contracts::TYPE_LIGHT_SWITCH) {
    // Keep callback short: copy to queue only
    Serial.printf("ESP-NOW packet received. Type: %d Length: %d\n", type, len);
    pushRxPacketFromISR(data, (uint16_t)len);
  }
}

void onEspNowSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  lastSendStatus = status;
  sendInFlight = false;

  if (status == ESP_NOW_SEND_SUCCESS) {
    sendOkCount++;
  } else {
    sendFailCount++;
  }
}

// ---------- Setup ESP-NOW ----------
bool initEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  uint8_t staMac[6];
  esp_read_mac(staMac, ESP_MAC_WIFI_STA);

  char macStr[18];
  snprintf(macStr, sizeof(macStr),
           "%02X:%02X:%02X:%02X:%02X:%02X",
           staMac[0], staMac[1], staMac[2],
           staMac[3], staMac[4], staMac[5]);

  Serial.print("ESP-NOW (STA) MAC: ");
  Serial.println(macStr);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    return false;
  }

  esp_now_register_recv_cb(onEspNowRecv);
  esp_now_register_send_cb(onEspNowSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastMAC, 6);
  peerInfo.channel = 0;     // current channel
  peerInfo.encrypt = false;

  if (esp_now_is_peer_exist(broadcastMAC)) {
    esp_now_del_peer(broadcastMAC);
  }

  // Broadcast peer must be added before sending broadcast packets
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("esp_now_add_peer failed");
    return false;
  }

  return true;
}

// ---------- UART framed reader ----------
// Reads one UART frame [len][type][value...] into uartBuf
bool readOneUartFrame(size_t &outTotalLen) {
  if (U.available() < 1) return false;

  int l = U.peek();
  if (l < 0) return false;

  uint8_t lenField = (uint8_t)l;

  if (lenField == 0 || lenField > MAX_LEN_FIELD) {
    // Drop one byte and try to resync
    U.read();
    return false;
  }

  size_t total = 1 + (size_t)lenField;
  if ((size_t)U.available() < total) return false;

  size_t r = U.readBytes(uartBuf, total);
  if (r != total) return false;

  outTotalLen = total;

  //Serial.println(outTotalLen);

  return true;
}

// ---------- Pump functions ----------
void pumpEspNowToUart() {
  Packet p;
  while (popRxPacket(p)) {
    // Rebuild UART frame as [len][payload...]
    uint8_t lenField = (uint8_t)p.len;
    U.write(&lenField, 1);
    U.write(p.data, p.len);
  }
}

void pumpUartToTxQueue() {
  size_t totalLen = 0;

  while (readOneUartFrame(totalLen)) {
    // uartBuf = [len][type][value...]
    // Queue only ESP-NOW payload = [type][value...]
    const uint8_t payloadLen = (uint8_t)(totalLen - 1);
    if (!pushTxPacket(uartBuf + 1, payloadLen)) {
      // queue full; packet dropped
    }
  }
}

void pumpTxQueueToEspNow() {
  if (sendInFlight) return;

  Packet p;
  if (!peekTxPacket(p)) return;

  dumpBytes(p.data, p.len);

  esp_err_t err = esp_now_send(broadcastMAC, p.data, p.len);
  if (err == ESP_OK) {
    sendInFlight = true;
    // Remove from queue only after successfully handing it to ESP-NOW
    dropTxPacket();
  } else {
    // If send call itself fails, drop this packet to avoid getting stuck forever.
    // Alternative: keep it and retry later.
    dropTxPacket();
    sendFailCount++;
  }
}

void dumpBytes(const uint8_t* data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (data[i] < 0x10) Serial.print("0");
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

// ---------- Arduino ----------
void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println("Shelly ESP-NOW <-> UART bridge");

  if (!initEspNow()) {
    Serial.println("ESPNOW init failed, restarting...");
    delay(1000);
    ESP.restart();
  }

  U.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
}

void loop() {
  pumpUartToTxQueue();
  pumpEspNowToUart();
  pumpTxQueueToEspNow();

  delay(1);
}