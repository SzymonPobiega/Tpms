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
    if (sensor == 8924691)  return 0;
    else if (sensor == 9580339)  return 1;
    else if (sensor == 9449267) return 2;
    else if (sensor == 6499363)  return 3;
    else if (sensor == 14745155)  return 4;
    else if (sensor == 2240579)  return 5;
    return 0;
}

bool processTpms(Contracts::TpmsPacket& pkt) {
    
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
