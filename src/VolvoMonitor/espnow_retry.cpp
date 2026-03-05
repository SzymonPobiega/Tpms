#include "espnow_retry.hpp"
#include <esp_wifi.h>
#include <string.h>

namespace {

constexpr int MAX_PEERS = 8;          // adjust to your needs
constexpr int MAX_PAYLOAD = 250;      // ESP-NOW payload limit is 250 bytes
constexpr uint8_t MAX_RETRIES = 5;

struct PeerCache {
  uint8_t mac[6];
  bool    used = false;

  // last packet
  uint8_t data[MAX_PAYLOAD];
  uint16_t len = 0;

  // retry state
  volatile bool retry_pending = false;   // set in send cb, consumed in poll()
  volatile bool fail_seen = false;       // for debug if you want
  uint8_t retries_left = 0;
  uint32_t next_retry_ms = 0;
};

PeerCache peers[MAX_PEERS];

int find_or_alloc_peer(const uint8_t *mac) {
  // find existing
  for (int i = 0; i < MAX_PEERS; i++) {
    if (peers[i].used && memcmp(peers[i].mac, mac, 6) == 0) return i;
  }
  // alloc new
  for (int i = 0; i < MAX_PEERS; i++) {
    if (!peers[i].used) {
      peers[i].used = true;
      memcpy(peers[i].mac, mac, 6);
      peers[i].len = 0;
      peers[i].retry_pending = false;
      peers[i].fail_seen = false;
      peers[i].retries_left = 0;
      peers[i].next_retry_ms = 0;
      return i;
    }
  }
  return -1;
}

int find_peer(const uint8_t *mac) {
  for (int i = 0; i < MAX_PEERS; i++) {
    if (peers[i].used && memcmp(peers[i].mac, mac, 6) == 0) return i;
  }
  return -1;
}

// Very simple backoff: 30ms, 60ms, 120ms, 240ms, 480ms (cap via MAX_RETRIES)
uint32_t backoff_ms(uint8_t attempt_index_from_0) {
  uint32_t base = 30U << attempt_index_from_0;
  if (base > 500) base = 500;
  return base;
}

// ---- send callback ----
// Note: signature differs between cores/IDF versions. Use the one that matches your build.
// Newer ESP-IDF: esp_now_send_cb_t: (const wifi_tx_info_t*, esp_now_send_status_t)
void on_send_cb(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  if (!info) return;

  // receiver address is in info->des_addr on current IDF builds
  const uint8_t *dst = info->des_addr;

  int idx = find_peer(dst);
  if (idx < 0) return;

  if (status == ESP_NOW_SEND_SUCCESS) {
    // success: clear any pending retry state
    peers[idx].retry_pending = false;
    peers[idx].retries_left = 0;
    return;
  }

  // FAIL: schedule retry (don’t resend from callback)
  peers[idx].fail_seen = true;

  // Only schedule if we have a cached packet
  if (peers[idx].len == 0) return;

  // If no retries armed yet, arm them
  if (peers[idx].retries_left == 0) {
    peers[idx].retries_left = MAX_RETRIES;
  }

  // schedule next retry soon
  uint8_t used_attempts = (uint8_t)(MAX_RETRIES - peers[idx].retries_left);
  peers[idx].next_retry_ms = millis() + backoff_ms(used_attempts);
  peers[idx].retry_pending = true;
}

} // namespace

bool espnow_retry_init() {
  // register send callback once ESP-NOW has been init'd
  return (esp_now_register_send_cb(on_send_cb) == ESP_OK);
}

esp_err_t espnow_send_cached(const uint8_t *peer_mac, const uint8_t *data, size_t len) {
  if (!peer_mac || !data || len == 0) return ESP_ERR_INVALID_ARG;
  if (len > MAX_PAYLOAD) return ESP_ERR_INVALID_SIZE;

  int idx = find_or_alloc_peer(peer_mac);
  if (idx < 0) return ESP_ERR_NO_MEM;

  // store last packet
  memcpy(peers[idx].data, data, len);
  peers[idx].len = (uint16_t)len;

  // reset retry state for this new "last packet"
  peers[idx].retry_pending = false;
  peers[idx].retries_left = 0;
  peers[idx].next_retry_ms = 0;

  return esp_now_send(peer_mac, data, len);
}

void espnow_retry_poll() {
  uint32_t now = millis();

  for (int i = 0; i < MAX_PEERS; i++) {
    if (!peers[i].used) continue;
    if (!peers[i].retry_pending) continue;
    if (peers[i].len == 0) { peers[i].retry_pending = false; continue; }
    if ((int32_t)(now - peers[i].next_retry_ms) < 0) continue; // not yet

    if (peers[i].retries_left == 0) {
      peers[i].retry_pending = false;
      continue;
    }

    peers[i].retries_left--;

    // resend cached packet
    esp_err_t err = esp_now_send(peers[i].mac, peers[i].data, peers[i].len);

    // schedule next retry if still failing (we don't know result yet; wait for cb)
    uint8_t used_attempts = (uint8_t)(MAX_RETRIES - peers[i].retries_left);
    peers[i].next_retry_ms = millis() + backoff_ms(used_attempts);

    // keep retry_pending true; callback will clear it on success
    (void)err;
  }
}