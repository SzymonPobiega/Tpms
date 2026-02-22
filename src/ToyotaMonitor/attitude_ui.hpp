#pragma once
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t *root;

    lv_obj_t *meter_l;
    lv_obj_t *meter_r;
    lv_meter_scale_t *scale_l;
    lv_meter_scale_t *scale_r;

    lv_meter_indicator_t *needle_l;
    lv_meter_indicator_t *needle_r;

    lv_meter_indicator_t *hl_l;   // yellow highlight arc (left)
    lv_meter_indicator_t *hl_r;   // yellow highlight arc (right)

    lv_obj_t *center;     // narrow center container
    lv_obj_t *car;        // lv_img or label placeholder
    lv_obj_t *title;      // "PITCH"/"ROLL"
    lv_obj_t *deg_label;  // big "39°"

    int16_t max_deg;      // e.g. 45
} AttitudeUI;

// Create the “two big side arcs + small centered car + big degree” control.
// size_px is the overall height. The control width is ~2*size_px.
AttitudeUI create_attitude_indicator(lv_obj_t *parent, int size_px);

// Update displayed value in degrees (positive = right highlight, negative = left highlight).
void attitude_set_value(AttitudeUI *ui, float deg);

void attitude_set_image_tilt(AttitudeUI *ui, float deg);

// Optional: if you later want to use a real image
// (call after create_attitude_indicator)
void attitude_set_car_image(AttitudeUI *ui, const void *src, uint16_t zoom_256);

void attitude_set_center_image(AttitudeUI *ui, const lv_img_dsc_t *img);

#ifdef __cplusplus
}
#endif
