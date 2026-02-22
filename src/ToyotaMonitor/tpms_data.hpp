#pragma once

#include <Arduino.h>

namespace tpms {

constexpr size_t kMaxSensors = 6;

extern volatile uint32_t sequence;
extern volatile uint32_t latestPressure[kMaxSensors];
extern volatile int16_t latestTemp[kMaxSensors];
extern volatile bool hasData;
extern volatile uint32_t lastUpdated[kMaxSensors];
extern volatile uint32_t totalPeriods[kMaxSensors];
extern volatile uint32_t last_update;
extern volatile uint32_t start_time;  // if you want it shared

struct TpmsPacket {
  uint32_t sequence;
  uint32_t sensorId;
  uint32_t pressure;
  int16_t temp;
} __attribute__((packed));

void initLink();
// uint32_t getIndex(uint32_t sensor);
bool tryReadTpms(TpmsPacket& pkt);

// ESP-NOW callback
// void onEspNowRecv(const esp_now_recv_info_t *info,
//                   const uint8_t *data, int len);

// // helper to init ESP-NOW and register callback
// bool initEspNow();
}
