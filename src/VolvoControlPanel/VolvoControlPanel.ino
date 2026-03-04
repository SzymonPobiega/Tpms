#include "src\waveshare/lvgl_port.h"           // LVGL porting functions for integration with ESP32
#include "ContractsInclude.hpp"
#include <esp_mac.h> 
#include <WiFi.h>
#include <esp_now.h>

static lv_obj_t *BAT_Label;      // Label to display battery voltage
char bat_v[20];                  // Buffer to store formatted battery voltage string

// Optional: track which mode is selected
typedef enum {
    MODE_FRONT_HIGH,
    MODE_FRONT_LOW,
    MODE_REAR_HIGH,
    MODE_REAR_LOW
} camera_mode_t;

static camera_mode_t g_mode = MODE_FRONT_HIGH;

void onEspNowSent(const wifi_tx_info_t* tx_info, esp_now_send_status_t status) {
    Serial.print("ESP-NOW send status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "SUCCESS" : "FAIL");
}

static void sidebar_btn_event_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    lv_obj_t *btn = lv_event_get_target(e);
    const char *txt = lv_label_get_text(lv_obj_get_child(btn, 0));

    if (!strcmp(txt, "Front High")) {
        g_mode = MODE_FRONT_HIGH;
        Serial.println("Mode: Front High");
        // TODO: your IO logic here
    } else if (!strcmp(txt, "Front Low")) {
        g_mode = MODE_FRONT_LOW;
        Serial.println("Mode: Front Low");
        // TODO: your IO logic here
    } else if (!strcmp(txt, "Rear High")) {
        g_mode = MODE_REAR_HIGH;
        Serial.println("Mode: Rear High");
        // TODO: your IO logic here
    } else if (!strcmp(txt, "Rear Low")) {
        g_mode = MODE_REAR_LOW;
        Serial.println("Mode: Rear Low");
        // TODO: your IO logic here
    }

    Contracts::CameraSelectPacket pkt;
    pkt.type = Contracts::TYPE_CAMERA_SELECT;
    pkt.input  = ((int16_t)g_mode)+1;

    esp_err_t res = esp_now_send(Contracts::LILYGO_CAMERA_MAC, (uint8_t*)&pkt, sizeof(pkt));
            Serial.print("  |  ESP-NOW send: ");
            Serial.println(res == ESP_OK ? "OK" : String("ERR ") + res);
}

static lv_obj_t* make_sidebar_btn(lv_obj_t *parent, const char *text, camera_mode_t mode)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_width(btn, lv_pct(100));
    lv_obj_set_height(btn, 60);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_center(lbl);

    lv_obj_add_event_cb(btn, sidebar_btn_event_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)mode);
    return btn;
}

void lvgl_ui(void)
{
    lv_obj_t *scr = lv_scr_act();

    // --- Sidebar container (left) ---
    lv_obj_t *sidebar = lv_obj_create(scr);
    lv_obj_set_size(sidebar, 150, 320);        // width, height
    lv_obj_set_pos(sidebar, 0, 0);

    // Layout: vertical stack
    lv_obj_set_flex_flow(sidebar, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(sidebar,
                          LV_FLEX_ALIGN_START,   // main axis
                          LV_FLEX_ALIGN_CENTER,  // cross axis
                          LV_FLEX_ALIGN_CENTER); // track axis
    lv_obj_set_style_pad_all(sidebar, 10, 0);
    lv_obj_set_style_pad_row(sidebar, 10, 0);

    // Helper to create a sidebar butto

    make_sidebar_btn(sidebar, "Front High", MODE_FRONT_HIGH);
    make_sidebar_btn(sidebar, "Front Low",  MODE_FRONT_LOW);
    make_sidebar_btn(sidebar, "Rear High",  MODE_REAR_HIGH);
    make_sidebar_btn(sidebar, "Rear Low",   MODE_REAR_LOW);

    // --- Battery label (center-ish, to the right of sidebar) ---
    BAT_Label = lv_label_create(scr);
    lv_obj_set_width(BAT_Label, LV_SIZE_CONTENT);
    lv_obj_set_height(BAT_Label, LV_SIZE_CONTENT);

    // Place it to the right of the sidebar
    lv_obj_set_pos(BAT_Label, 170, 120);
    lv_label_set_text(BAT_Label, "BAT:3.7V");

    // Style the battery label
    lv_obj_set_style_text_color(BAT_Label, lv_color_hex(0xFFA500),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(BAT_Label, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(BAT_Label, &lv_font_montserrat_44,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
}

/**
 * @brief Main setup function.
 */
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
        lvgl_ui();
        lvgl_port_unlock();
    }
}

void loop() {

    delay(100);
}