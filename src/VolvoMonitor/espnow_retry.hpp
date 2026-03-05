#pragma once
#include <Arduino.h>
#include <esp_now.h>

bool espnow_retry_init();

/// Call instead of esp_now_send.
/// Stores a copy of the packet for that peer and sends it.
esp_err_t espnow_send_cached(const uint8_t *peer_mac, const uint8_t *data, size_t len);

/// Call from loop() frequently (e.g. every iteration).
void espnow_retry_poll();