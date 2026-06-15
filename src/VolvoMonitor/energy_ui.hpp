#pragma once
#include <lvgl.h>
#include <stdint.h>

namespace CamperUI {

enum class LightGroup : uint8_t {
    Interior = 0,
    Exterior = 1
};

enum class Actuator : uint8_t {
    Hatch = 0,
    Roof  = 1
};

enum class Direction : uint8_t {
    Up   = 0,
    Down = 1
};

struct Callbacks {
    // Called when a single light toggle changes
    void (*onLightChanged)(LightGroup group, uint8_t index0to3, bool on) = nullptr;

    // Called when user triggers "All Off" (after long press)
    void (*onAllOff)() = nullptr;

    // called when finger goes down on UP/DOWN
    void (*onActuatorPressed)(Actuator a, Direction d) = nullptr;

    // called when finger lifts OR press is lost
    void (*onActuatorReleased)(Actuator a, Direction d) = nullptr;
};

// Create UI on the active screen (lv_scr_act()).
void init(lv_obj_t* parent, lv_coord_t width, lv_coord_t height, const Callbacks& cb);

// Optional: call these from your app when values update
void setSocPercent(uint8_t soc);            // 0..100
void setRemainingAh(float ah);              // e.g. 62.0
void setChargeCurrentA(float a);            // e.g. 12.4

// Optional: update states from logic (if you need external sync)
void setLightState(LightGroup group, uint8_t index0to3, bool on);

} // namespace CamperUI