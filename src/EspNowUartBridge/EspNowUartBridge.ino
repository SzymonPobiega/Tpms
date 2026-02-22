#include <WiFi.h>
#include <esp_now.h>
#include <esp_mac.h> 
#include "ContractsInclude.hpp"

// -------------------- USER CONFIG --------------------
static const uint8_t broadcastMAC[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF }; // <-- Brodcast

static const int ESPNOW_CHANNEL = 1;   // must match all peers

static const int UART_RX_PIN = 16;     // GPIO16
static const int UART_TX_PIN = 17;     // GPIO17
static const uint32_t UART_BAUD = 115200;

// Framing: [len:1][type:1][value:len-1], total bytes on wire = 1 + len
static const uint8_t MAX_LEN_FIELD = 250;   // max "len" value we accept
static const size_t  MAX_MSG_BYTES = 1 + MAX_LEN_FIELD;
// -----------------------------------------------------

// Use Serial1 for UART bridge
HardwareSerial &U = Serial1;

// Small RX buffer for UART -> ESPNOW
static uint8_t uartBuf[MAX_MSG_BYTES];

// ---------- ESPNOW callbacks ----------
void onEspNowRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (!data || len <= 0) return;

  // Forward as-is to UART (exact bytes, no modification)
  U.write(data, len);
}

void onEspNowSent(const wifi_tx_info_t* tx_info, esp_now_send_status_t status) {
}

// ---------- Setup ESPNOW ----------
bool initEspNow() {
    WiFi.mode(WIFI_STA);

    WiFi.disconnect();

    // Read the real STA MAC from efuse
    uint8_t staMac[6];
    esp_read_mac(staMac, ESP_MAC_WIFI_STA);

    char macStr[18];
    snprintf(macStr, sizeof(macStr),
            "%02X:%02X:%02X:%02X:%02X:%02X",
            staMac[0], staMac[1], staMac[2],
            staMac[3], staMac[4], staMac[5]);

    Serial.print("ESP-NOW (STA) MAC: ");
    Serial.println(macStr);

    // Now init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed!");
        return false;
    }


    esp_now_register_recv_cb(onEspNowRecv);
    esp_now_register_send_cb(onEspNowSent);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastMAC, 6);
    peerInfo.channel = ESPNOW_CHANNEL;
    peerInfo.encrypt = false;

    // Add peer (replace if already exists)
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("esp_now_add_peer failed");
        return false;
    }

    return true;
}

// ---------- UART framing reader ----------
// Non-blocking-ish: attempts to read a full framed message; returns true when a full message is in uartBuf
bool readOneUartMessage(size_t &outLen) {
  // Need at least 1 byte for len
  if (U.available() < 1) return false;

  int l = U.peek();
  if (l < 0) return false;

  uint8_t lenField = (uint8_t)l;

  // Validate
  if (lenField == 0 || lenField > MAX_LEN_FIELD) {
    // bad length -> drop one byte to resync
    U.read();
    return false;
  }

  // Need total bytes available: 1 + lenField
  size_t total = 1 + (size_t)lenField;
  if ((size_t)U.available() < total) return false;

  // Read the full frame
  size_t r = U.readBytes(uartBuf, total);
  if (r != total) return false;

  outLen = total;
  return true;
}

// ---------- Arduino ----------
void setup() {
    Serial.begin(115200);

    Serial.println("Hello...");

    if (!initEspNow()) {
        Serial.println("ESPNOW init failed, restarting...");
        delay(1000);
        ESP.restart();
    }

    Serial.println("Bridge ready: ESPNOW <-> UART");

    delay(1000);
    // UART on GPIO16/17
    U.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
    
    //From now on we can't print because GPIO pins are used to connections
}

void loop() {
  // UART -> ESPNOW
  size_t msgLen = 0;
  if (readOneUartMessage(msgLen)) {
    // Send as-is
    esp_err_t err = esp_now_send(broadcastMAC, uartBuf, msgLen);
    if (err != ESP_OK) {
      // If send fails, you can optionally log
      // Serial.printf("esp_now_send err=%d\n", (int)err);
    }
  }

  // Let background WiFi/ESPNOW do its thing
  delay(1);
}
