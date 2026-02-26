#include "compas_ui.hpp"
#include <Arduino.h>
#include "esp_heap_caps.h"
#include <stdio.h>

static inline int wrap360(int d) {
    d %= 360;
    if (d < 0) d += 360;
    return d;
}

static const char* dir_from_deg(int deg) {
    // 8-wind: N, NE, E, SE, S, SW, W, NW
    static const char* dirs[] = {"N","NE","E","SE","S","SW","W","NW"};
    int idx = ((deg + 22) / 45) & 7;
    return dirs[idx];
}

static void draw_ruler(compass_strip_t* ui, int heading_deg)
{
    if(!ui->canvas || !ui->cbuf) return;

    const lv_color_t bg     = lv_color_hex(0x0B1020);
    const lv_color_t white  = lv_color_hex(0xEAF0FF);
    const lv_color_t dim    = lv_color_hex(0x93A3B8);
    const lv_color_t orange = lv_color_hex(0xF29B2E);

    lv_obj_t* c = ui->canvas;
    const int W = ui->w;
    const int H = ui->canvas_h;
    const int cx = W / 2;

    lv_canvas_fill_bg(c, bg, LV_OPA_COVER);

    const int y_base = (int)(H * 0.7f);

    const int tick_small = (int)(H * 0.22f);
    const int tick_med   = (int)(H * 0.32f);
    const int tick_big   = (int)(H * 0.44f);

    // Zoom: degrees across width
    const float deg_across = 160.0f;
    const float px_per_deg = (float)W / deg_across;
    const int span_deg = (int)(deg_across * 0.5f); // -80..+80

    // Line style
    lv_draw_line_dsc_t line;
    lv_draw_line_dsc_init(&line);
    line.color = white;
    line.width = 3;
    line.opa = LV_OPA_COVER;

    // Label styles
    lv_draw_label_dsc_t lbl_major;
    lv_draw_label_dsc_init(&lbl_major);
    lbl_major.color = orange;
    lbl_major.opa = LV_OPA_COVER;
    lbl_major.font = &lv_font_montserrat_18;

    lv_draw_label_dsc_t lbl_minor;
    lv_draw_label_dsc_init(&lbl_minor);
    lbl_minor.color = dim;
    lbl_minor.opa = LV_OPA_COVER;
    lbl_minor.font = &lv_font_montserrat_16;

    for (int rel = -span_deg; rel <= span_deg; rel++) {
        int deg = wrap360(heading_deg + rel);

        if ((deg % 5) != 0) continue;

        int x = (int)(cx + rel * px_per_deg);
        if (x < -24 || x > W + 24) continue;

        int len = tick_small;
        if ((deg % 30) == 0) len = tick_big;
        else if ((deg % 10) == 0) len = tick_med;

        // LVGL8 canvas line: points[] + point_cnt
        lv_point_t pts[2] = {
            { (lv_coord_t)x, (lv_coord_t)y_base },
            { (lv_coord_t)x, (lv_coord_t)(y_base - len) }
        };
        lv_canvas_draw_line(c, pts, 2, &line);

        // Major numbers every 30°
        if ((deg % 30) == 0) {
            char t[8];
            lv_snprintf(t, sizeof(t), "%d", deg);

            lv_point_t sz;
            lv_txt_get_size(&sz, t, lbl_major.font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);

            int tx = x - sz.x / 2;
            int ty = y_base - len - sz.y - 6;
            if (ty < 0) ty = 0;   // prevent clipping
            // LVGL8 canvas text: (canvas, x, y, max_w, dsc, text)
            lv_canvas_draw_text(c, tx, ty, sz.x + 6, &lbl_major, t);
        }

        // Cardinals
        const char* card = NULL;
        if      (deg == 0)   card = "N";
        else if (deg == 90)  card = "E";
        else if (deg == 180) card = "S";
        else if (deg == 270) card = "W";

        if (card) {
            lv_point_t sz;
            lv_txt_get_size(&sz, card, lbl_minor.font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);

            int tx = x - sz.x / 2;
            int ty = (int)(H * 0.80f);

            lv_canvas_draw_text(c, tx, ty, sz.x + 6, &lbl_minor, card);
        }
    }

    // ---- Center marker (triangle) ----
    // Try to draw a FILLED polygon (best). If your LVGL build lacks it,
    // comment this block and use the 3-line fallback below.
#if LV_USE_DRAW_SW
    lv_draw_rect_dsc_t poly; // used by canvas polygon on some builds
    lv_draw_rect_dsc_init(&poly);
    poly.bg_color = orange;
    poly.bg_opa = LV_OPA_COVER;
    poly.border_opa = LV_OPA_TRANSP;

    // lv_canvas_draw_polygon exists in LVGL 8 canvas in many builds:
    // void lv_canvas_draw_polygon(canvas, points, point_cnt, &draw_dsc)
    lv_point_t tri[3] = {
        { (lv_coord_t)cx,        (lv_coord_t)(y_base + 8)  },
        { (lv_coord_t)(cx - 12), (lv_coord_t)(y_base - 12) },
        { (lv_coord_t)(cx + 12), (lv_coord_t)(y_base - 12) }
    };

    // If this fails to compile on your exact package, use the fallback below.
    lv_canvas_draw_polygon(c, tri, 3, &poly);
#else
    // Fallback: outline triangle with 3 lines
    lv_draw_line_dsc_t o;
    lv_draw_line_dsc_init(&o);
    o.color = orange;
    o.width = 3;
    o.opa = LV_OPA_COVER;

    lv_point_t a[2] = {{(lv_coord_t)cx, (lv_coord_t)(y_base + 8)}, {(lv_coord_t)(cx - 12), (lv_coord_t)(y_base - 12)}};
    lv_point_t b[2] = {{(lv_coord_t)(cx - 12), (lv_coord_t)(y_base - 12)}, {(lv_coord_t)(cx + 12), (lv_coord_t)(y_base - 12)}};
    lv_point_t d[2] = {{(lv_coord_t)(cx + 12), (lv_coord_t)(y_base - 12)}, {(lv_coord_t)cx, (lv_coord_t)(y_base + 8)}};

    lv_canvas_draw_line(c, a, 2, &o);
    lv_canvas_draw_line(c, b, 2, &o);
    lv_canvas_draw_line(c, d, 2, &o);
#endif
}

extern "C" {

void compass_strip_create(compass_strip_t* ui, lv_obj_t* parent, int x, int y, int w, int h)
{
    if (!ui) return;
    lv_memset_00(ui, sizeof(*ui));

    if (h > 150) h = 150;
    if (h < 80)  h = 80;   // keep it usable
    if (w < 200) w = 200;

    ui->w = (int16_t)w;
    ui->h = (int16_t)h;
    ui->last_heading = -1000;

    ui->root = lv_obj_create(parent);
    lv_obj_set_pos(ui->root, x, y);
    lv_obj_set_size(ui->root, w, h);
    lv_obj_clear_flag(ui->root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui->root, lv_color_hex(0x0B1020), 0);
    lv_obj_set_style_border_width(ui->root, 0, 0);
    lv_obj_set_style_pad_all(ui->root, 10, 0);

    // Top row
    int top_h = 60;
    if (top_h > h - 45) top_h = h - 45;
    if (top_h < 45) top_h = 45;

    ui->canvas_h = (int16_t)(h - top_h - 5);
    if (ui->canvas_h < 30) ui->canvas_h = 30;

    lv_obj_t* top = lv_obj_create(ui->root);
    lv_obj_set_size(top, w, top_h);
    lv_obj_align(top, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(top, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(top, 0, 0);
    lv_obj_set_style_pad_all(top, 0, 0);

    ui->lbl_deg = lv_label_create(top);
    lv_label_set_text(ui->lbl_deg, "0°");
    lv_obj_set_style_text_color(ui->lbl_deg, lv_color_hex(0xF29B2E), 0);
    lv_obj_set_style_text_font(ui->lbl_deg, &lv_font_montserrat_40, 0);
    lv_obj_align(ui->lbl_deg, LV_ALIGN_LEFT_MID, 20, -10);

    ui->lbl_dir = lv_label_create(top);
    lv_label_set_text(ui->lbl_dir, "N");
    lv_obj_set_style_text_color(ui->lbl_dir, lv_color_hex(0xEAF0FF), 0);
    lv_obj_set_style_text_font(ui->lbl_dir, &lv_font_montserrat_40, 0);
    lv_obj_align_to(ui->lbl_dir, ui->lbl_deg, LV_ALIGN_OUT_RIGHT_MID, 60, 0);

    ui->lbl_alt = lv_label_create(top);
    lv_label_set_text(ui->lbl_alt, "0 m");
    lv_obj_set_style_text_color(ui->lbl_alt, lv_color_hex(0xEAF0FF), 0);
    lv_obj_set_style_text_font(ui->lbl_alt, &lv_font_montserrat_40, 0);
    lv_obj_align(ui->lbl_alt, LV_ALIGN_RIGHT_MID, -20, -10);

    // Bottom ruler canvas
    ui->canvas = lv_canvas_create(ui->root);
    lv_obj_set_size(ui->canvas, w, ui->canvas_h);
    lv_obj_align(ui->canvas, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_clear_flag(ui->canvas, LV_OBJ_FLAG_SCROLLABLE);

    size_t bytes = (size_t)w * (size_t)ui->canvas_h * sizeof(lv_color_t);

    Serial.printf("Compass canvas request: %u bytes\n", (unsigned)bytes);

    // Allocate explicitly from PSRAM
    ui->cbuf = (lv_color_t*)heap_caps_malloc(
        bytes,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
    );

    if(!ui->cbuf) {
        Serial.println("Compass PSRAM alloc FAILED!");
        return;   // VERY important: don't continue
    }

    Serial.printf("Compass PSRAM alloc OK. Free heap now: %u\n",
                (unsigned)ESP.getFreeHeap());

    lv_canvas_set_buffer(ui->canvas, ui->cbuf, w, ui->canvas_h, LV_IMG_CF_TRUE_COLOR);

    // Initial draw
    compass_strip_set_heading(ui, 0);
    compass_strip_set_altitude(ui, 0);
}

void compass_strip_destroy(compass_strip_t* ui)
{
    if (!ui) return;

    if (ui->root) {
        lv_obj_del(ui->root);
        ui->root = NULL;
    }

    if (ui->cbuf) {
        heap_caps_free(ui->cbuf);
        ui->cbuf = NULL;
    }
}

void compass_strip_set_heading(compass_strip_t* ui, int heading_deg)
{
    if (!ui || !ui->root) return;

    if(!ui->canvas || !ui->cbuf) return;

    heading_deg = wrap360(heading_deg);
    if (ui->last_heading == heading_deg) return;
    ui->last_heading = (int16_t)heading_deg;

    char buf[16];
    lv_snprintf(buf, sizeof(buf), "%d°", heading_deg);
    lv_label_set_text(ui->lbl_deg, buf);
    lv_label_set_text(ui->lbl_dir, dir_from_deg(heading_deg));

    draw_ruler(ui, heading_deg);
}

void compass_strip_set_altitude(compass_strip_t* ui, int altitude_m)
{
    if (!ui || !ui->root) return;

    char buf[32];
    lv_snprintf(buf, sizeof(buf), "%d m", altitude_m);
    lv_label_set_text(ui->lbl_alt, buf);
}

lv_obj_t* compass_strip_obj(compass_strip_t* ui)
{
    return ui ? ui->root : NULL;
}

} // extern "C"