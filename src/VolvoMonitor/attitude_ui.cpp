#include "attitude_ui.hpp"
#include <string.h>
#include <math.h>

static inline float clampf(float v, float lo, float hi)
{
    if(v < lo) return lo;
    if(v > hi) return hi;
    return v;
}

static inline void make_static(lv_obj_t *o)
{
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(o, LV_SCROLLBAR_MODE_OFF);
}

static void style_ticks_only(lv_obj_t *m)
{
    // Keep object, remove theme styles
    lv_obj_remove_style_all(m);
    lv_obj_set_style_bg_opa(m, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(m, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(m, 0, 0);
    make_static(m);
}

// Build one side meter (square) with inward-facing semicircle ticks
static void build_side_meter(lv_obj_t *parent,
                            lv_obj_t **out_meter,
                            lv_meter_scale_t **out_scale,
                            lv_meter_indicator_t **out_needle,
                            lv_meter_indicator_t **out_hl,
                            int size_sq,
                            bool right_side,
                            int max_deg)
{
    lv_obj_t *m = lv_meter_create(parent);
    style_ticks_only(m);
    lv_obj_set_size(m, size_sq, size_sq);

    lv_meter_scale_t *sc = lv_meter_add_scale(m);

    // 90° arc: +/-45° around the side direction
    // Left centered at 180° -> 135..225
    // Right centered at   0° -> 315..45
    if(right_side) {
        lv_meter_set_scale_range(m, sc, max_deg, -max_deg, 90, 315);
    } else {
        lv_meter_set_scale_range(m, sc, -max_deg, max_deg, 90, 135);
    }

    // Dense ticks like your reference image
    lv_meter_set_scale_ticks(m, sc, 19, 2, 10, lv_color_black());      
    lv_meter_set_scale_major_ticks(m, sc, 3, 3, 18, lv_color_black(), 10);

    // Needle (yellow)
    lv_meter_indicator_t *needle =
        lv_meter_add_needle_line(m, sc, 4, lv_palette_main(LV_PALETTE_YELLOW), -18);

    // Yellow highlight arc: we will set start/end dynamically in attitude_set_value()
    lv_meter_indicator_t *hl =
        lv_meter_add_arc(m, sc, 10, lv_palette_main(LV_PALETTE_YELLOW), 0);

    // Start with no highlight
    lv_meter_set_indicator_start_value(m, hl, 0);
    lv_meter_set_indicator_end_value(m, hl, 0);

    *out_meter = m;
    *out_scale = sc;
    *out_needle = needle;
    *out_hl = hl;
}

AttitudeUI create_attitude_indicator(lv_obj_t *parent, int size_px)
{
    AttitudeUI ui;
    memset(&ui, 0, sizeof(ui));

    ui.max_deg = 45;

    // Overall footprint: same as “two gauges side-by-side”
    const lv_coord_t W = (lv_coord_t)(size_px * 2);
    const lv_coord_t H = (lv_coord_t)(size_px);

    // Titles removed — reclaim vertical space
    const lv_coord_t title_h = 0;
    ui.title = nullptr;

    // Meters must fit inside remaining height
    const lv_coord_t meter_sz = H - title_h;  // <-- fixes bottom trimming
    const lv_coord_t y0 = title_h;

    // Root
    ui.root = lv_obj_create(parent);
    lv_obj_remove_style_all(ui.root);
    lv_obj_clear_flag(ui.root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(ui.root, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(ui.root, W, H);
    lv_obj_center(ui.root);

    // Build left/right meters (SQUARE so arcs are big)
    build_side_meter(ui.root, &ui.meter_l, &ui.scale_l, &ui.needle_l, &ui.hl_l,
                     meter_sz, false, ui.max_deg);

    build_side_meter(ui.root, &ui.meter_r, &ui.scale_r, &ui.needle_r, &ui.hl_r,
                     meter_sz, true, ui.max_deg);

    // target_inner_gap:
    //   0  -> meters touch at the inner edge
    //  <0  -> meters overlap by -gap pixels (recommended: -10..-30)
    const lv_coord_t target_inner_gap = -250;  // overlap by 20 px

    lv_coord_t base_inner_gap = W - 2 * meter_sz;
    lv_coord_t overlap = base_inner_gap - target_inner_gap;   // becomes larger if target is negative
    if(overlap < 0) overlap = 0;

    lv_coord_t shift = overlap / 2;
    lv_obj_set_pos(ui.meter_l, shift, y0);
    lv_obj_set_pos(ui.meter_r, W - meter_sz - shift, y0);

    // Center image goes directly on ROOT, above meters (no clipping container)
    ui.car = nullptr; // set later via attitude_set_center_image()

    // Degree label at bottom center
    // ui.deg_label = lv_label_create(ui.root);
    // lv_label_set_text(ui.deg_label, "--°");
    // lv_obj_set_style_text_color(ui.deg_label, lv_color_black(), 0);
    // lv_obj_align(ui.deg_label, LV_ALIGN_BOTTOM_MID, 0, -6);

    // Initialize
    attitude_set_value(&ui, 0.0f);

    return ui;
}


void attitude_set_center_image(AttitudeUI *ui, const lv_img_dsc_t *img)
{
    if(!ui || !ui->root || !img) return;

    if(ui->car) {
        lv_obj_del(ui->car);
        ui->car = nullptr;
    }

    ui->car = lv_img_create(ui->root);
    lv_img_set_src(ui->car, img);

    lv_obj_clear_flag(ui->car, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(ui->car, LV_SCROLLBAR_MODE_OFF);

    lv_img_set_pivot(ui->car,
                 img->header.w / 2,
                 img->header.h / 2);

    // Place it
    lv_obj_align(ui->car, LV_ALIGN_CENTER, 0, 0);

    // Make sure it draws above the meters
    lv_obj_move_foreground(ui->car);
}

void attitude_set_image_tilt(AttitudeUI *ui, float deg)
{
    if(!ui || !ui->car) return;

    // 0.1 degree units
    int16_t a10 = (int16_t)lroundf(deg * 10.0f);
    lv_img_set_angle(ui->car, a10);

    // Keep it on top
    lv_obj_move_foreground(ui->car);
}



void attitude_set_car_image(AttitudeUI *ui, const void *src, uint16_t zoom_256)
{
    if(!ui || !ui->center) return;

    // Delete placeholder if it was a label
    if(ui->car) lv_obj_del(ui->car);

    ui->car = lv_img_create(ui->center);
    lv_img_set_src(ui->car, src);
    lv_img_set_zoom(ui->car, zoom_256); // 256=100%, 128=50%, etc.
    lv_obj_align(ui->car, LV_ALIGN_CENTER, 0, -10);
}

void attitude_set_value(AttitudeUI *ui, float deg)
{
    if(!ui) return;

    float d = clampf(deg, -(float)ui->max_deg, (float)ui->max_deg);

    // Needles follow the actual degree
    if(ui->meter_l && ui->needle_l) lv_meter_set_indicator_value(ui->meter_l, ui->needle_l, (int32_t)d);
    if(ui->meter_r && ui->needle_r) lv_meter_set_indicator_value(ui->meter_r, ui->needle_r, (int32_t)-d);

    // Highlight: negative -> left side, positive -> right side
    int32_t a = (int32_t)lroundf(fabsf(d));

    // Reset both
    if(ui->meter_l && ui->hl_l) {
        lv_meter_set_indicator_start_value(ui->meter_l, ui->hl_l, 0);
        lv_meter_set_indicator_end_value(ui->meter_l, ui->hl_l, 0);
    }
    if(ui->meter_r && ui->hl_r) {
        lv_meter_set_indicator_start_value(ui->meter_r, ui->hl_r, 0);
        lv_meter_set_indicator_end_value(ui->meter_r, ui->hl_r, 0);
    }

    if(d < 0) {
        // left highlight: from -a to 0 (or 0 to -a)
        lv_meter_set_indicator_start_value(ui->meter_l, ui->hl_l, -a);
        lv_meter_set_indicator_end_value(ui->meter_l, ui->hl_l, 0);

        lv_meter_set_indicator_start_value(ui->meter_r, ui->hl_r, a);
        lv_meter_set_indicator_end_value(ui->meter_r, ui->hl_r, 0);

    } else if(d > 0) {
        // right highlight: from 0 to +a
        lv_meter_set_indicator_start_value(ui->meter_l, ui->hl_l, 0);
        lv_meter_set_indicator_end_value(ui->meter_l, ui->hl_l, a);

        lv_meter_set_indicator_start_value(ui->meter_r, ui->hl_r, 0);
        lv_meter_set_indicator_end_value(ui->meter_r, ui->hl_r, -a);
    }

    // Degree label
    if(ui->deg_label) {
        char buf[16];
        lv_snprintf(buf, sizeof(buf), "%d°", (int)lroundf(d));
        lv_label_set_text(ui->deg_label, buf);
    }

    attitude_set_image_tilt(ui, d);
}
