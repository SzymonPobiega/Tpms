#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include "esp_system.h"
#include <esp_mac.h> 
#include <WiFi.h>
#include <esp_now.h>
#include "ContractsInclude.hpp"

using namespace Contracts;

NimBLEScan* pBLEScan = nullptr;

//WIT BLE
static NimBLEUUID SVC_FFE5("0000FFE5-0000-1000-8000-00805F9A34FB");
static NimBLEUUID CH_FFE4("0000FFE4-0000-1000-8000-00805F9A34FB");

static const char* TARGET_MAC = "e0:3b:b5:fe:27:8e";

static NimBLEClient* client = nullptr;
static NimBLERemoteCharacteristic* chNotify = nullptr;

static std::vector<uint8_t> rxBuf;

void onEspNowSent(const wifi_tx_info_t* tx_info, esp_now_send_status_t status) {
    //Serial.print("ESP-NOW send status: ");
    //Serial.println(status == ESP_NOW_SEND_SUCCESS ? "SUCCESS" : "FAIL");
}

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("GYRO gateway");

    WiFi.mode(WIFI_STA);        // required for ESP-NOW
    WiFi.disconnect();          // just to be safe

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
        Serial.println("❌ ESP-NOW init failed!");
        return;
    }

    // New-style callback signature for your core:
    esp_now_register_send_cb(onEspNowSent);

    // Add peer (your ESP32-S3)
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, ToyotaMonitorMAC, 6);
    peerInfo.channel = 0;   // 0 = current Wi-Fi channel
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add ESP-NOW peer!");
        while (true) { delay(1000); }
    }

    NimBLEDevice::init("GYRO-Gateway");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);

    if (!connectWT901()) {
      Serial.println("❌ Failed to connect to WT901");
      return;
    }
    //startScan();
    Serial.println("✅ Setup complete.");
}

// ---------- Helpers ----------

static inline int16_t le_i16(const uint8_t* p) {
  return (int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static bool checksum_ok_11(const uint8_t* f) {
  uint8_t sum = 0;
  for (int i = 0; i < 10; i++) sum = (uint8_t)(sum + f[i]);
  return sum == f[10];
}

static void printHex(const uint8_t* data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    Serial.printf("%02X", data[i]);
    if (i + 1 < len) Serial.print(' ');
  }
  Serial.println();
}

static void consumeFrames() {
  constexpr size_t FRAME_LEN = 20;

  while (rxBuf.size() >= FRAME_LEN) {

    // resync to 0x55 0x61
    if (!(rxBuf[0] == 0x55 && rxBuf[1] == 0x61)) {
      rxBuf.erase(rxBuf.begin());
      continue;
    }

    const uint8_t* f = rxBuf.data();

    // Decode angles at fixed offsets in the 0x61 frame:
    int16_t r_raw = le_i16(&f[14]);
    int16_t p_raw = le_i16(&f[16]);
    int16_t y_raw = le_i16(&f[18]);

    constexpr float SCALE = 180.0f / 32768.0f;
    float roll  = r_raw * SCALE;
    float pitch = p_raw * SCALE;
    float yaw   = y_raw * SCALE;

    Serial.printf("RPY: %.2f %.2f %.2f\n", roll, pitch, yaw);

    GyroAnglePacket pkt;
    pkt.roll_deg  = (int16_t)(roll  * 100.0f);
    pkt.pitch_deg = (int16_t)(pitch * 100.0f);
    pkt.yaw_deg   = (int16_t)(yaw   * 100.0f);
    esp_now_send(ToyotaMonitorMAC, (uint8_t*)&pkt, sizeof(pkt));

    rxBuf.erase(rxBuf.begin(), rxBuf.begin() + FRAME_LEN);
  }
}


static void onNotify(NimBLERemoteCharacteristic*, uint8_t* data, size_t len, bool) {
  //printHex(data, len);
  rxBuf.insert(rxBuf.end(), data, data + len);
  consumeFrames();
}


static bool connectWT901() {
  NimBLEAddress addr(std::string(TARGET_MAC), BLE_ADDR_RANDOM );
  if (!client) {
    client = NimBLEDevice::createClient();
    client->setConnectionParams(24, 40, 0, 2000);
  }

  Serial.print("Connecting to ");
  Serial.println(TARGET_MAC);

  client->connect(addr);
  if (!client->isConnected()) {
    Serial.println("❌ Connect failed");
    return false;
  }

  NimBLERemoteService* svc = client->getService(SVC_FFE5);
  if (!svc) {
    Serial.println("❌ FFE5 service not found");
    return false;
  }

  chNotify = svc->getCharacteristic(CH_FFE4);
  if (!chNotify) {
    Serial.println("❌ FFE4 notify characteristic not found");
    return false;
  }

  if (!chNotify->canNotify()) {
    Serial.println("❌ FFE4 has no notify");
    return false;
  }

  Serial.println("Subscribing to FFE4 notify...");
  if (!chNotify->subscribe(true, onNotify)) {
    Serial.println("❌ Subscribe failed");
    return false;
  }

  Serial.println("✅ Connected + subscribed (WT901)");
  return true;
}


void loop() {
  if (!client || !client->isConnected()) {
      connectWT901();
  }
  delay(1000);
}
