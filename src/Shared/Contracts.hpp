#pragma once
#include <stdint.h>

#if defined(__GNUC__)
  #define PACKED __attribute__((packed))
#else
  #define PACKED
#endif

namespace Contracts {

  static constexpr uint8_t SYNC1 = 0xAA;
  static constexpr uint8_t SYNC2 = 0x55;
  static constexpr uint8_t TYPE_GYRO = 0x03;

  uint8_t S3_5_1_MAC[] = { 0x50, 0x78, 0x7D, 0x13, 0x22, 0xF8 };
  uint8_t P4_4_1_MAC[] = { 0x9C, 0x13, 0x9E, 0xC3, 0xFF, 0x60 };

  struct PACKED GyroAnglePacket {
    int16_t roll_deg;   // centi-degrees
    int16_t pitch_deg;  // centi-degrees
    int16_t yaw_deg;    // centi-degrees
  };

  static_assert(sizeof(GyroAnglePacket) == 6, "GyroAnglePacket must be 6 bytes");
}
