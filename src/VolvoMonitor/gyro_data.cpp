#include "gyro_data.hpp"

namespace gyro {

volatile int16_t roll_deg = 0;
volatile int16_t pitch_deg = 0;
volatile int16_t yaw_deg = 0;
volatile int16_t height = 0;

void processGyroAngle(Contracts::GyroAnglePacket& pkt) {
  roll_deg = pkt.roll_deg / 100;
  pitch_deg = pkt.pitch_deg / 100;
  yaw_deg = (360 - (pkt.yaw_deg / 100) + 90) % 360;
  height = pkt.height;
}

void processGyroHeight(Contracts::GyroHeightPacket& pkt) {
  height = pkt.height;
}
}
