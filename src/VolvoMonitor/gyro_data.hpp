#pragma once

#include <Arduino.h>
#include "ContractsInclude.hpp"

namespace gyro {

extern volatile int16_t roll_deg;
extern volatile int16_t pitch_deg;
extern volatile int16_t yaw_deg;
extern volatile int16_t height;

void processGyroAngle(Contracts::GyroAnglePacket& pkt);
void processGyroHeight(Contracts::GyroHeightPacket& pkt);
}
