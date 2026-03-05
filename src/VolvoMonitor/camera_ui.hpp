#pragma once
#include "lvgl.h"

typedef void (*CameraCallback)(uint8_t camera_id);

void setup_camera_tab(lv_obj_t *parent, int disp_w, int disp_h);

// Register callback fired when a button is selected
void camera_ui_set_callback(CameraCallback cb);