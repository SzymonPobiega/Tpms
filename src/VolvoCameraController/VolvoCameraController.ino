#include <WiFi.h>
#include <esp_mac.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "ContractsInclude.hpp"

using namespace Contracts;

static const int RELAY_PINS[4] = {21, 19, 18, 5};
static const bool RELAY_ACTIVE_LOW = false;

void setRelays(uint16_t which)
{
  for (int i = 0; i < 4; i++) {

    bool on = (which == i + 1);

    if (RELAY_ACTIVE_LOW)
      digitalWrite(RELAY_PINS[i], on ? LOW : HIGH);
    else
      digitalWrite(RELAY_PINS[i], on ? HIGH : LOW);
  }
}

void onReceive(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len)
{
  CameraSelectPacket pkt;
  memcpy(&pkt, incomingData, sizeof(pkt));

  Serial.print("Received relay command: ");
  Serial.println(pkt.input);

  if (pkt.input >= 1 && pkt.input <= 4)
    setRelays(pkt.input);
}

void setup()
{
  Serial.begin(115200);
  Serial.println("GYRO gateway");

  for (int i = 0; i < 4; i++)
  {
    pinMode(RELAY_PINS[i], OUTPUT);
  }

  setRelays(1);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  uint8_t staMac[6];
  esp_err_t err = esp_read_mac(staMac, ESP_MAC_WIFI_STA);
  if (err != ESP_OK) {
      Serial.printf("❌ esp_read_mac failed: %d\n", err);
      return;
  }

  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
            staMac[0], staMac[1], staMac[2],
            staMac[3], staMac[4], staMac[5]);

  Serial.print("ESP-NOW (STA) MAC: ");
  Serial.println(macStr);

  if (esp_now_init() != ESP_OK) {
      Serial.println("❌ ESP-NOW init failed!");
      return;
  }

  uint8_t efuse[6];
  esp_efuse_mac_get_default(efuse);
  Serial.printf("eFuse MAC:        %02X:%02X:%02X:%02X:%02X:%02X\n",
                efuse[0], efuse[1], efuse[2], efuse[3], efuse[4], efuse[5]);

  esp_now_register_recv_cb(onReceive);

  Serial.println("✅ Setup complete");
}

void loop()
{
}