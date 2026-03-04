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
  static constexpr int16_t TYPE_TPMS = 0x02;
  static constexpr int16_t TYPE_GYRO_ANGLE = 0x03;
  static constexpr int16_t TYPE_GYRO_HEIGHT = 0x04;
  static constexpr int16_t TYPE_GYRO_BMS = 0x05;
  static constexpr int16_t TYPE_CAMERA_SELECT = 0x06;

  //S3
  inline constexpr uint8_t S3_5_1_MAC[] = { 0x50, 0x78, 0x7D, 0x13, 0x22, 0xF8 };

  //Volvo camera control LilyGO
  inline constexpr uint8_t LILYGO_CAMERA_MAC[] = { 0x94, 0x51, 0xDC, 0x47, 0xAE, 0xC4 };

  //P4
  inline constexpr uint8_t P4_4_1_MAC[] = { 0x9C, 0x13, 0x9E, 0xC3, 0xFF, 0x60 };

  struct PACKED GyroAnglePacket {
    int16_t type;
    int16_t roll_deg;   // centi-degrees
    int16_t pitch_deg;  // centi-degrees
    int16_t yaw_deg;    // centi-degrees
    int16_t height;     // centi-meters
  };

  struct PACKED CameraSelectPacket {
    int16_t type;
    int16_t input;
  };

  struct PACKED BmsPacket {
    int16_t type;
    int16_t volts;   // centi-volts
    int16_t amps;  // centi-amps
    int16_t soc_percent;    // percent
    int16_t remaining;     // centi-amp-h
    int16_t temp; //deci-deg
  };

  struct PACKED GyroHeightPacket {
    int16_t type;
    int16_t height;     // centi-meters
  };

  struct PACKED TpmsPacket {
    int16_t type;
    uint32_t sequence;
    uint32_t sensorId;
    uint32_t pressure;
    int16_t temp;
  };

  static_assert(sizeof(GyroAnglePacket) == 10, "GyroAnglePacket must be 10 bytes");
  static_assert(sizeof(GyroHeightPacket) == 4, "GyroHeightPacket must be 4 bytes");
  static_assert(sizeof(TpmsPacket) == 16, "TpmsPacket must be 16 bytes");
  static_assert(sizeof(BmsPacket) == 12, "BmsPacket must be 12 bytes");
}
