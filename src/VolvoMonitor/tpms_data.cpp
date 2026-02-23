#include "tpms_data.hpp"

namespace tpms {

volatile uint32_t sequence = 0;
volatile uint32_t latestPressure[kMaxSensors] = {0};
volatile int16_t latestTemp[kMaxSensors]     = {0};
volatile bool     hasData                 = false;
volatile uint32_t lastUpdated[kMaxSensors]    = {0};
volatile uint32_t totalPeriods[kMaxSensors]   = {0};
volatile uint32_t last_update                 = 0;
volatile uint32_t start_time                  = 0;

uint32_t getIndex(uint32_t sensor)
{
    if (sensor == 7549971)  return 0;
    else if (sensor == 1126691)  return 1;
    else if (sensor == 14554163) return 2;
    else if (sensor == 4592707)  return 3;
    return 0;
}

static constexpr uint8_t SYNC1 = 0xAA;
static constexpr uint8_t SYNC2 = 0x55;
static constexpr uint8_t TYPE_TPMS = 0x01;

// Choose the P4 pins you wired to:
static constexpr int P4_RX_PIN = 32; // wired from C6_U0TXD
static constexpr int P4_TX_PIN = 35; // wired to C6_U0RXD (optional)

HardwareSerial Link(1);

void initLink() {
    Link.begin(115200, SERIAL_8N1, P4_RX_PIN, P4_TX_PIN);
}

static bool readByteWithTimeout(Stream& s, uint8_t& out, uint32_t timeoutMs = 50) {
  uint32_t start = millis();
  while (!s.available()) {
    if (millis() - start > timeoutMs) return false;
    delay(1);
  }
  out = (uint8_t)s.read();
  return true;
}

static bool readExact(Stream& s, uint8_t* buf, size_t n, uint32_t timeoutMs = 50) {
  for (size_t i = 0; i < n; i++) {
    if (!readByteWithTimeout(s, buf[i], timeoutMs)) return false;
  }
  return true;
}


static bool recvTpms(TpmsPacket& pkt) {
  // Find sync AA 55
  uint8_t b;
  while (Link.available()) {
    if (!readByteWithTimeout(Link, b)) return false;
    if (b != SYNC1) continue;

    uint8_t b2;
    if (!readByteWithTimeout(Link, b2)) return false;
    if (b2 != SYNC2) continue;

    uint8_t len;
    if (!readByteWithTimeout(Link, len)) return false;

    // Expect len = 1(type) + payload
    const uint8_t expectedLen = (uint8_t)(1 + sizeof(TpmsPacket));
    if (len != expectedLen) {
      // Skip unknown frame
      uint8_t trash[64];
      if (len <= sizeof(trash)) {
        if (!readExact(Link, trash, len)) return false;
      } else {
        // drain in chunks
        uint8_t tmp[64];
        uint16_t remaining = len;
        while (remaining) {
          uint8_t chunk = remaining > sizeof(tmp) ? sizeof(tmp) : remaining;
          if (!readExact(Link, tmp, chunk)) return false;
          remaining -= chunk;
        }
      }
      continue;
    }

    uint8_t type;
    if (!readByteWithTimeout(Link, type)) return false;
    if (type != TYPE_TPMS) {
      // Skip payload
      uint8_t trash[sizeof(TpmsPacket)];
      if (!readExact(Link, trash, sizeof(TpmsPacket))) return false;
      continue;
    }

    // Read payload into struct
    return readExact(Link, (uint8_t*)&pkt, sizeof(TpmsPacket));
  }
  return false;
}

bool tryReadTpms(TpmsPacket& pkt) {
    if (!recvTpms(pkt))
    {
        return false;
    }

    uint32_t now = millis();
    uint32_t idx = getIndex(pkt.sensorId);
    if (idx >= kMaxSensors) {
        return false;
    }

    if (pkt.sequence > sequence) {
        sequence = pkt.sequence;
        latestPressure[idx] = pkt.pressure;
        latestTemp[idx]     = pkt.temp;
        //We only increase the total number of periods per sensor if the packet is a new one (not a retransmission)
        totalPeriods[idx]++;
        lastUpdated[idx] = now;
        hasData = true;
        Serial.print("Dupa!");
    }
    
    last_update = now;
    return true;
}

} // namespace tpms
