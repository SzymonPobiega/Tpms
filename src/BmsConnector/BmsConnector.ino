#include <Arduino.h>
#include <NimBLEDevice.h>
#include "esp_system.h"
#include <esp_mac.h> 
#include <WiFi.h>
#include <esp_now.h>

NimBLEScan* pBLEScan = nullptr;

uint8_t espNowPeerMac[] = { 0x50, 0x78, 0x7D, 0x13, 0x22, 0xF8 };

//BMS BLE
static NimBLEUUID SVC_FF00("0000ff00-0000-1000-8000-00805f9b34fb");
static NimBLEUUID CH_FF01("0000ff01-0000-1000-8000-00805f9b34fb");
static NimBLEUUID CH_FF02("0000ff02-0000-1000-8000-00805f9b34fb");

//WIT BLE
static NimBLEUUID SVC_FFE5("0000FFE5-0000-1000-8000-00805F9A34FB");
static NimBLEUUID CH_FFE4("0000FFE4-0000-1000-8000-00805F9A34FB");

static const char* TARGET_MAC = "A5:C2:37:62:EF:6E";

static NimBLEClient* client = nullptr;
static NimBLERemoteCharacteristic* chNotify = nullptr;
static NimBLERemoteCharacteristic* chWrite = nullptr;

static std::vector<uint8_t> rxBuf;

#pragma pack(push, 1)
struct TpmsPacket {
    uint32_t sequence;
    uint32_t sensorId;
    uint32_t pressure;  // raw 32-bit value as you decode it
    uint16_t temp;      // raw 16-bit value
};


struct BmsBasicInfoPacket {
    int32_t packVoltage_cV;    // centivolts (0.01 V)
    int32_t current_cA;        // centiamps (0.01 A), signed
    uint8_t soc_percent;       // %
    int32_t remaining_cAh;     // centiamp-hours (0.01 Ah)
    int32_t temperature_dC;    // deci-degC (0.1 °C)
};
#pragma pack(pop)


static constexpr uint8_t SYNC1 = 0xAA;
static constexpr uint8_t SYNC2 = 0x55;
static constexpr uint8_t TYPE_TPMS = 0x01;
static constexpr uint8_t TYPE_BMS = 0x02;

static TpmsPacket pendingMessage;

void onEspNowSent(const wifi_tx_info_t* tx_info, esp_now_send_status_t status) {
    Serial.print("ESP-NOW send status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "SUCCESS" : "FAIL");
}

class MyScanCallbacks : public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
        String name = advertisedDevice->getName().c_str();

        // Ignore unnamed devices
        if (!name.length()) {
            return;
        }

        Serial.print(name);
        Serial.print("  |  ");
        Serial.print(advertisedDevice->getAddress().toString().c_str());

        // Filter: only devices whose name starts with "TPMS"
        if (!name.startsWith("TPMS")) {
            return;
        }

        const std::vector<uint8_t>& payload = advertisedDevice->getPayload();
        size_t length = payload.size();

        if (length >= 32) {
            uint32_t pressure = 0;
            pressure |= (uint32_t)payload[17];
            pressure |= (uint32_t)payload[18] << 8;
            pressure |= (uint32_t)payload[19] << 16;
            pressure |= (uint32_t)payload[20] << 24;

            uint16_t temp = 0;
            temp |= (uint16_t)payload[21];
            temp |= (uint16_t)payload[22] << 8;

            uint32_t sensorId = 0;
            sensorId |= (uint32_t)payload[14];
            sensorId |= (uint32_t)payload[15] << 8;
            sensorId |= (uint32_t)payload[16] << 16;

            Serial.print("  |  Sensor: ");
            Serial.print(sensorId);
            Serial.print("  |  Pressure: ");
            Serial.print(pressure);
            Serial.print("  |  Temp: ");
            Serial.print(temp);

            pendingMessage.sensorId = sensorId;
            pendingMessage.pressure = pressure;
            pendingMessage.temp     = temp;
            pendingMessage.sequence++;

            esp_err_t res = esp_now_send(espNowPeerMac, (uint8_t*)&pendingMessage, sizeof(pendingMessage));
            Serial.print("  |  ESP-NOW send: ");
            Serial.println(res == ESP_OK ? "OK" : String("ERR ") + res);

            const uint8_t len = (uint8_t)(1 + sizeof(TpmsPacket));
            Serial2.write(SYNC1);
            Serial2.write(SYNC2);
            Serial2.write(len);
            Serial2.write(TYPE_TPMS);
            Serial2.write((const uint8_t*)&pendingMessage, sizeof(TpmsPacket));

        } else {
            Serial.print("  |  Payload too short (");
            Serial.print(length);
            Serial.print(")");
        }

        Serial.println();
    }

    void onScanEnd(const NimBLEScanResults& results, int reason) override {
        Serial.println("Scan ended");
    }
};

// ==== Setup ==== //
void setup() {

  NimBLEDevice::init("ESP32-BMS");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);

  if (!connectBMS()) {
    Serial.println("❌ Failed to connect to BMS");
  }
}
void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("TPMS gateway: scanning TPMS* and forwarding via ESP-NOW.");

    // Binary link to other ESP over UART2 on GPIO16/17
    Serial2.begin(115200, SERIAL_8N1, 16, 17);

    //Init the sequence to 1 so that the Monitor can detect the first packet
    pendingMessage.sequence = 1;

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
        Serial.println("ESP-NOW init failed!");
        while (true) { delay(1000); }
    }

    // New-style callback signature for your core:
    esp_now_register_send_cb(onEspNowSent);

    // Add peer (your ESP32-S3)
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, espNowPeerMac, 6);
    peerInfo.channel = 0;   // 0 = current Wi-Fi channel
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add ESP-NOW peer!");
        while (true) { delay(1000); }
    }

    // --- BLE scanner init (NimBLE) --- //
    NimBLEDevice::init("TPMS-Gateway");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); // TX power for scan requests, etc.

    pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setScanCallbacks(new MyScanCallbacks(), /*wantDuplicates=*/true);
    pBLEScan->setFilterPolicy(BLE_HCI_SCAN_FILT_NO_WL);
    pBLEScan->setActiveScan(true);  // request scan response
    
    Serial.println("Setup complete.");
}

// ---------- Helpers ----------

static uint16_t be16(const uint8_t* p) {
  return (uint16_t(p[0]) << 8) | p[1];
}

static int16_t be16s(const uint8_t* p) {
  return int16_t((uint16_t(p[0]) << 8) | p[1]);
}

bool parseBasicInfo03(const uint8_t* frame, size_t len, BmsBasicInfoPacket& out) {
  if (len < 41) return false;
  if (frame[0] != 0xDD || frame[1] != 0x03 || frame[len - 1] != 0x77) return false;

  const uint8_t status = frame[2];
  const uint8_t payloadLen = frame[3];
  if (status != 0x00 || payloadLen != 0x22) return false;

  // Payload begins at frame[4]
  const uint8_t* pl = &frame[4];

  // Offsets inside payload (JBD 0x03):
  // 0..1  pack voltage (0.01V)
  // 2..3  current (0.01A signed)
  // 4..5  remaining cap (0.01Ah)
  // 16    SW version
  // 17    SOC
  // 18    MOS state
  // 19    cell count
  // 20    temp sensor count
  // 21..22 temperature (0.1K)

  out.packVoltage_cV = (int32_t)((uint16_t(pl[0]) << 8) | pl[1]);
  out.current_cA     = (int32_t)(int16_t((uint16_t(pl[2]) << 8) | pl[3]));
  out.remaining_cAh  = (int32_t)((uint16_t(pl[4]) << 8) | pl[5]);
  out.soc_percent    = pl[19];

  const uint16_t temp_raw = (uint16_t(pl[23]) << 8) | pl[24];
  out.temperature_dC = (int32_t)temp_raw - 2731; // 0.1K -> 0.1°C

  return true;
}


static void printHex(const uint8_t* data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    Serial.printf("%02X", data[i]);
    if (i + 1 < len) Serial.print(' ');
  }
}

static void consumeFrames() {
  while (true) {
    // Find frame start
    auto itStart = std::find(rxBuf.begin(), rxBuf.end(), 0xDD);
    if (itStart == rxBuf.end()) {
      rxBuf.clear();
      return;
    }

    // Drop junk before start
    if (itStart != rxBuf.begin())
      rxBuf.erase(rxBuf.begin(), itStart);

    // Find frame end
    auto itEnd = std::find(rxBuf.begin() + 1, rxBuf.end(), 0x77);
    if (itEnd == rxBuf.end())
      return; // wait for more data

    std::vector<uint8_t> frame(rxBuf.begin(), itEnd + 1);
    rxBuf.erase(rxBuf.begin(), itEnd + 1);

    // ---- Handle frame ----
    if (frame.size() >= 2 && frame[1] == 0x03) {
      BmsBasicInfoPacket info;
      if (parseBasicInfo03(frame.data(), frame.size(), info)) {
        Serial.printf(
          "BMS: V=%ld.%02ldV  I=%ld.%02ldA  SOC=%u%%  Rem=%ld.%02ldAh  T=%ld.%1ldC\n",
          info.packVoltage_cV / 100, abs(info.packVoltage_cV % 100),
          info.current_cA / 100, abs(info.current_cA % 100),
          info.soc_percent,
          info.remaining_cAh / 100, abs(info.remaining_cAh % 100),
          info.temperature_dC / 10, abs(info.temperature_dC % 10)
        );
      }
    }
  }
}


// ---------- Notify callback ----------
static void onNotify(NimBLERemoteCharacteristic*, uint8_t* data, size_t len, bool b) {
  //Serial.print("[RX chunk] ");
  //printHex(data, len);
  //Serial.println();

  rxBuf.insert(rxBuf.end(), data, data + len);
  consumeFrames();
}

// ---------- Connect ----------
static bool connectBMS() {
  NimBLEAddress addr(std::string(TARGET_MAC), BLE_ADDR_PUBLIC);
  client = NimBLEDevice::createClient();

  client->setConnectionParams(12, 12, 0, 2000);

  Serial.print("Connecting to ");
  Serial.println(TARGET_MAC);

  if (!client->connect(addr)) {
    Serial.println("❌ Connect failed");
    return false;
  }

  NimBLERemoteService* svc = client->getService(SVC_FF00);
  if (!svc) {
    Serial.println("❌ FF00 service not found");
    return false;
  }

  chNotify = svc->getCharacteristic(CH_FF01);
  chWrite  = svc->getCharacteristic(CH_FF02);

  if (!chNotify || !chWrite) {
    Serial.println("❌ Characteristics not found");
    return false;
  }

  if (!chNotify->canNotify()) {
    Serial.println("❌ FF01 has no notify");
    return false;
  }

  Serial.println("Subscribing to FF01 notify...");
  if (!chNotify->subscribe(true, onNotify)) {
    Serial.println("❌ Subscribe failed");
    return false;
  }

  Serial.println("✅ Connected + subscribed");
  return true;
}

// ---------- Send command ----------
static void sendCmd(const uint8_t* cmd, size_t len) {
  if (!client || !client->isConnected()) return;

//   Serial.print("[TX] ");
//   printHex(cmd, len);
//   Serial.println();

  bool noResp = chWrite->canWriteNoResponse();
  chWrite->writeValue(cmd, len, !noResp);
}

// ---------- Known-good commands ----------
static void sendBasic03() {
  const uint8_t cmd[] = {0xDD, 0xA5, 0x03, 0x00, 0xFF, 0xFD, 0x77};
  sendCmd(cmd, sizeof(cmd));
}

void loop() {
    // static uint32_t lastScanStart = 0;
    // uint32_t now = millis();

    // // If not currently scanning, start a 30-second scan every ~5s gap
    // if (!pBLEScan->isScanning() && (now - lastScanStart > 5000)) {
    //     Serial.println("Starting TPMS scan...");
    //     // duration=30 (seconds), isContinue=false, restart=false
    //     pBLEScan->start(120000, false, false);
    //     lastScanStart = now;
    // }

    // delay(5000);

    // //PING - send last received measurement
    // esp_err_t res = esp_now_send(espNowPeerMac, (uint8_t*)&pendingMessage, sizeof(pendingMessage));

    static uint32_t lastPoll = 0;

    if (!client || !client->isConnected()) {
        delay(1000);
        return;
    }

    uint32_t now = millis();
    if (now - lastPoll >= 1000) {
        lastPoll = now;

        sendBasic03();
        //delay(100);
        //sendTemps06();
    }

    delay(10);
}
