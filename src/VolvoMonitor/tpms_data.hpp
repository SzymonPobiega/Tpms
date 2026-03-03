#pragma once

#include <Arduino.h>
#include "ContractsInclude.hpp"

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

bool processTpms(Contracts::TpmsPacket& pkt);
}
