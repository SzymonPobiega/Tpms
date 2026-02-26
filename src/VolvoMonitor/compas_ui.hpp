#pragma once
#include "lvgl.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t* root;

    lv_obj_t* lbl_deg;
    lv_obj_t* lbl_dir;
    lv_obj_t* lbl_alt;

    lv_obj_t* canvas;
    lv_color_t* cbuf;

    int16_t w;
    int16_t h;
    int16_t canvas_h;

    int16_t last_heading;
} compass_strip_t;

/**
 * Create the compass strip UI.
 * parent: LVGL parent
 * x,y: position
 * w: recommended 740 (800 - 60)
 * h: will be clamped to <= 150
 */
void compass_strip_create(compass_strip_t* ui, lv_obj_t* parent, int x, int y, int w, int h);

/** Delete the UI and free internal buffer. */
void compass_strip_destroy(compass_strip_t* ui);

void compass_strip_set_heading(compass_strip_t* ui, int heading_deg);   // 0..359
void compass_strip_set_altitude(compass_strip_t* ui, int altitude_m);   // meters

lv_obj_t* compass_strip_obj(compass_strip_t* ui);

#ifdef __cplusplus
} // extern "C"
#endif