#include "src\waveshare/lvgl_port.h"           // LVGL porting functions for integration with ESP32
#include "ContractsInclude.hpp"
#include <esp_mac.h> 
#include <WiFi.h>
#include <esp_now.h>
#include "CamperUI.hpp"

// Example callbacks
static void onLightChanged(CamperUI::LightGroup group, uint8_t idx, bool on) {
    Serial.printf("Light group=%u idx=%u -> %s\n",
                  (unsigned)group, (unsigned)idx, on ? "ON" : "OFF");
    // TODO: your IO / ESP-NOW logic here
}

static void onAllOff() {
    Serial.println("ALL OFF triggered");
    // TODO: your IO / ESP-NOW logic here
}

static void onActPressed(CamperUI::Actuator a, CamperUI::Direction d) {
    Serial.printf("PRESS act=%u dir=%u\n", (unsigned)a, (unsigned)d);
    // start motor/relay
}

static void onActReleased(CamperUI::Actuator a, CamperUI::Direction d) {
    Serial.printf("RELEASE act=%u dir=%u\n", (unsigned)a, (unsigned)d);
    // stop motor/relay
}

void onEspNowSent(const wifi_tx_info_t* tx_info, esp_now_send_status_t status) {
    Serial.print("ESP-NOW send status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "SUCCESS" : "FAIL");
}

static void build_ui()
{
    CamperUI::Callbacks cb;
    cb.onLightChanged = onLightChanged;
    cb.onAllOff = onAllOff;
    cb.onActuatorPressed = onActPressed;
    cb.onActuatorReleased = onActReleased;
    CamperUI::init(cb);

    // Example: set some live values
    CamperUI::setSocPercent(78);
    CamperUI::setRemainingAh(62.0f);
    CamperUI::setChargeCurrentA(12.4f);
    CamperUI::setDischargeCurrentA(6.8f);
}

void setup() {
    Serial.begin(115200);

    Serial.println();
    Serial.println("Control panel");

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
    memcpy(peerInfo.peer_addr, Contracts::LILYGO_CAMERA_MAC, 6);
    peerInfo.channel = 0;   // 0 = current Wi-Fi channel
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add ESP-NOW peer!");
        while (true) { delay(1000); }
    }

    static esp_lcd_panel_handle_t panel_handle = NULL;
    static esp_lcd_touch_handle_t tp_handle = NULL;

    tp_handle = touch_gt911_init();
    panel_handle = waveshare_esp32_s3_rgb_lcd_init();
    wavesahre_rgb_lcd_bl_on();

    ESP_ERROR_CHECK(lvgl_port_init(panel_handle, tp_handle));

    ESP_LOGI(TAG, "Display LVGL UI");

    if (lvgl_port_lock(-1)) {
        build_ui();
        lvgl_port_unlock();
    }
}

void loop() {

    delay(100);
}