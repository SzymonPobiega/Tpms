#include <WiFi.h>
#include <esp_mac.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "ContractsInclude.hpp"

using namespace Contracts;

static const int RELAY_PINS[4] = {21, 19, 18, 5};
static const int INPUT_PINS[4] = {22, 23, 32, 33};
static int enabled[4] = {false, false, false, false};
static const uint8_t broadcast_mac[] = {0xff,0xff,0xff,0xff,0xff,0xff};
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 5000;

void toggleRelay(uint16_t which)
{
  bool on = !enabled[which];
  enabled[which] = on;

  Serial.print("New state");
  Serial.println(on);

  digitalWrite(RELAY_PINS[which], on ? HIGH : LOW);

  sendUpdate();
}

void setRelay(uint16_t which, int16_t state)
{
  bool on = state;
  enabled[which] = on;

  Serial.print("New state");
  Serial.println(on);

  digitalWrite(RELAY_PINS[which], on ? HIGH : LOW);
}

void onReceive(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len)
{
  WorkLightsSwitchPacket pkt;
  memcpy(&pkt, incomingData, sizeof(pkt));

  if (pkt.type != TYPE_WORKLIGHT_SWITCH) {
    Serial.print("Received invalid command: ");
    Serial.println(pkt.type);
    return;
  }

  Serial.print("Received relay command: ");
  Serial.print(pkt.type);
  Serial.print(pkt.input);
  Serial.println(pkt.state);

  if (pkt.input >= 1 && pkt.input <= 4)
    setRelay(pkt.input - 1, pkt.state);
}

void sendUpdate()
{
  Contracts::WorkLightsStatusPacket pkt;
  pkt.type = Contracts::TYPE_WORKLIGHT_STATUS;
  pkt.relay1 = enabled[0];
  pkt.relay2 = enabled[1];
  pkt.relay3 = enabled[2];
  pkt.relay4 = enabled[3];

  esp_err_t res = esp_now_send(broadcast_mac, (uint8_t*)&pkt, sizeof(pkt));
  if (res != ESP_OK) {
    Serial.print("ESP-NOW send: ");
    Serial.println(res == ESP_OK ? "OK" : String("ERR ") + res);
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Work lights controller");

  for (int i = 0; i < 4; i++)
  {
    pinMode(INPUT_PINS[i], INPUT_PULLUP);
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], LOW); // OFF
  }

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

  esp_now_register_recv_cb(onReceive);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcast_mac, 6);
  peerInfo.channel = 0;      // use current Wi-Fi channel
  peerInfo.encrypt = false;  // broadcast encryption not supported

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add broadcast peer");
    return;
  }

  Serial.println("✅ Setup complete");
} 

void loop()
{
  if (millis() - lastUpdate >= updateInterval) {
    lastUpdate = millis();
    sendUpdate();
  }

  for (int i = 0; i < 4; i++)
  {
    if (digitalRead(INPUT_PINS[i]) == LOW) {
      Serial.print("Input active:");
      Serial.println(i);
      toggleRelay(i);
      delay(1000);
      return;
    }
  }

  delay(10);
}