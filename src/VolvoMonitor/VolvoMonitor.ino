#include <Arduino.h>
#include "lvgl.h"
#include <esp_display_panel.hpp>
#include <WiFi.h>
#include <esp_mac.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "gauge_ui.hpp"
#include "tpms_data.hpp"
#include "attitude_ui.hpp"
#include "pitch.c"
#include "roll.c"

using namespace esp_panel::board;
using namespace esp_panel::drivers;

#if LV_COLOR_DEPTH != 16
#error "Set LV_COLOR_DEPTH to 16 (RGB565) in lv_conf.h"
#endif

// Keep these somewhere global if you need to update them later
static AttitudeUI pitch_ui;
static AttitudeUI roll_ui;

extern const lv_img_dsc_t pitch_img;
extern const lv_img_dsc_t roll_img;

Board *board = nullptr;
Touch *touch = nullptr; 
LCD *lcd = nullptr;

// ---- LVGL glue ----
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1 = nullptr;
static lv_color_t *buf2 = nullptr;
static int lcd_w = 0, lcd_h = 0;

static void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  int32_t x = area->x1;
  int32_t y = area->y1;
  int32_t w = area->x2 - area->x1 + 1;
  int32_t h = area->y2 - area->y1 + 1;
  if (w <= 0 || h <= 0) {
    lv_disp_flush_ready(disp);
    return;
  }

  lcd->drawBitmap(x, y, w, h, reinterpret_cast<const uint8_t *>(color_p));
  lv_disp_flush_ready(disp);
}

static void my_touch_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
  if (!touch) {
    data->state = LV_INDEV_STATE_RELEASED;
    return;
  }

  // Read up to 1 point (you can read more if you want gestures)
  esp_panel::drivers::TouchPoint p;
  int n = touch->readPoints(&p, 1, 0);   // (points, max_points, timeout_ms)

  if (n > 0) {
    data->state = LV_INDEV_STATE_PRESSED;

    // --- coordinate transform for your LVGL SW rotation (ROT_90) ---
    // physical: (p.x, p.y) in LCD coordinates (lcd_w x lcd_h)
    // logical:  rotated 90° clockwise
    int x = p.x;
    int y = p.y;

    data->point.x = x;
    data->point.y = y;

  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Volvo monitor");

  board = new Board();
  assert(board->begin());

  lcd = board->getLCD();
  if (!lcd) {
    Serial0.println("LCD is not available");
    while (true) { delay(1000); }
  }

  touch = board->getTouch();

  if (auto bl = board->getBacklight()) {
    bl->on();
    bl->setBrightness(50);
  }

  lcd_w = lcd->getFrameWidth();
  lcd_h = lcd->getFrameHeight();
  Serial0.printf("LCD: %dx%d\n", lcd_w, lcd_h);

  lv_init();

  // Two line-buffers (40 lines each)
  const int lines = 40;
  size_t buf_pixels = lcd_w * lines;
  buf1 = (lv_color_t *)heap_caps_malloc(buf_pixels * sizeof(lv_color_t), MALLOC_CAP_8BIT);
  buf2 = (lv_color_t *)heap_caps_malloc(buf_pixels * sizeof(lv_color_t), MALLOC_CAP_8BIT);
  assert(buf1 && buf2);

  lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buf_pixels);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = lcd_w;
  disp_drv.ver_res = lcd_h;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;

  disp_drv.sw_rotate = 1;
  disp_drv.rotated = LV_DISP_ROT_90;

  lv_disp_drv_register(&disp_drv);

  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touch_read;
  lv_indev_drv_register(&indev_drv);
    
  // Get the (now valid) default display & its active screen
  lv_disp_t *disp = lv_disp_get_default();
  lv_obj_t *scr = lv_disp_get_scr_act(disp);

  int disp_w = lv_disp_get_hor_res(disp);
  int disp_h = lv_disp_get_ver_res(disp);

  // Screen background
  lv_obj_set_style_bg_color(scr, lv_color_make(155, 155, 155), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

  /* Create the tabview as top-level */
  lv_obj_t *tv = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 50);  // 50 = tab bar height
  lv_obj_set_size(tv, disp_w, disp_h);

  /* Create tabs */
  lv_obj_t *tab1 = lv_tabview_add_tab(tv, "Gauges");
  lv_obj_t *tab2 = lv_tabview_add_tab(tv, "Tab2");
  lv_obj_t *tab3 = lv_tabview_add_tab(tv, "Tab3");

  lv_obj_t *content = lv_tabview_get_content(tv);
  lv_obj_set_style_pad_all(content, 0, 0);
  lv_obj_set_style_border_width(content, 0, 0);

  /* Remove padding/border from the tab page itself */
  lv_obj_set_style_pad_all(tab1, 0, 0);
  lv_obj_set_style_border_width(tab1, 0, 0);
  lv_obj_set_style_radius(tab1, 0, 0);

  lv_obj_t *cont = lv_obj_create(tab1);

  lv_obj_clear_flag(tab1, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_set_size(cont, lv_pct(100), lv_pct(100));
  lv_obj_set_style_pad_all(cont, 0, 0);
  lv_obj_set_style_border_width(cont, 0, 0);
  lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

  static lv_coord_t col_dsc[] = {
    LV_GRID_FR(1), LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t row_dsc[] = {
    LV_GRID_FR(4),  // Row 0: top gauges
    LV_GRID_FR(4),  // Row 1: bottom gauges
    LV_GRID_FR(2),  // Row 2: controls
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(cont, col_dsc, row_dsc);

  int cell_w = disp_w / 2;
  int cell_h = disp_h / 3;
  int s = (cell_w < cell_h ? cell_w : cell_h) * 9 / 10;

  // Create 4 gauges and place them in the grid
  gauges.clear();
  gauges.reserve(4);
  for (int r = 0; r < 2; ++r) {
    for (int c = 0; c < 2; ++c) {

      // One container per cell
      lv_obj_t *cell = lv_obj_create(cont);
      lv_obj_set_style_bg_opa(cell, LV_OPA_TRANSP, 0);  // transparent
      lv_obj_remove_style_all(cell);
      lv_obj_set_size(cell, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

      lv_obj_set_grid_cell(
        cell,
        LV_GRID_ALIGN_CENTER, c, 1,
        LV_GRID_ALIGN_CENTER, r, 1);

      GaugeUI g = create_pressure_gauge(cell, s);

      gauges.push_back(g);
    }
  }

  /* Remove padding/border from the tab page itself */
  lv_obj_set_style_pad_all(tab2, 0, 0);
  lv_obj_set_style_border_width(tab2, 0, 0);
  lv_obj_set_style_radius(tab2, 0, 0);

  lv_obj_t *cont2 = lv_obj_create(tab2);

  lv_obj_clear_flag(tab2, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(cont2, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_set_size(cont2, lv_pct(100), lv_pct(100));
  lv_obj_set_style_pad_all(cont2, 0, 0);
  lv_obj_set_style_border_width(cont2, 0, 0);
  lv_obj_set_style_bg_opa(cont2, LV_OPA_TRANSP, 0);
  lv_obj_clear_flag(cont2, LV_OBJ_FLAG_SCROLLABLE);

  setup_tab2(cont2, disp_w, disp_h);

  init_gauge_timer();
  //tpms::initLink();

  Serial0.printf("Dupa!!!!");
}

void setup_tab2(lv_obj_t *cont, int disp_w, int disp_h) {
  static lv_coord_t col_dsc[] = {
    LV_GRID_FR(1), LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
  };
  static lv_coord_t row_dsc[] = {
    LV_GRID_FR(4),  // Row 0: top (Pitch)
    LV_GRID_FR(4),  // Row 1: bottom (Roll)
    LV_GRID_FR(2),  // Row 2: controls
    LV_GRID_TEMPLATE_LAST
  };
  lv_obj_set_grid_dsc_array(cont, col_dsc, row_dsc);

  // Match your size logic
  int cell_w = disp_w / 2;
  int cell_h = disp_h / 3;
  int s = (cell_w < cell_h ? cell_w : cell_h) * 9 / 10;

  // ---- Pitch cell (row 0, span 2 columns) ----
  lv_obj_t *cell_pitch = lv_obj_create(cont);
  lv_obj_remove_style_all(cell_pitch);
  lv_obj_set_style_bg_opa(cell_pitch, LV_OPA_TRANSP, 0);
  lv_obj_set_size(cell_pitch, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

  lv_obj_set_grid_cell(
    cell_pitch,
    LV_GRID_ALIGN_CENTER, 0, 2,
    LV_GRID_ALIGN_CENTER, 0, 1);

  int s_att = (int)(s * 1.3f);
  pitch_ui = create_attitude_indicator(cell_pitch, s_att);
  attitude_set_center_image(&pitch_ui, &pitch_img);

  // ---- Roll cell (row 1, span 2 columns) ----
  lv_obj_t *cell_roll = lv_obj_create(cont);
  lv_obj_remove_style_all(cell_roll);
  lv_obj_set_style_bg_opa(cell_roll, LV_OPA_TRANSP, 0);
  lv_obj_set_size(cell_roll, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

  lv_obj_set_grid_cell(
    cell_roll,
    LV_GRID_ALIGN_CENTER, 0, 2,
    LV_GRID_ALIGN_CENTER, 1, 1);

  roll_ui = create_attitude_indicator(cell_roll, s_att);
  attitude_set_center_image(&roll_ui, &roll_img);

  // ---- Controls row (same as your other screen) ----
  lv_obj_t *cell_led = lv_obj_create(cont);
  lv_obj_remove_style_all(cell_led);
  lv_obj_set_style_bg_opa(cell_led, LV_OPA_TRANSP, 0);
  lv_obj_set_size(cell_led, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

  lv_obj_set_grid_cell(
    cell_led,
    LV_GRID_ALIGN_CENTER, 0, 1,
    LV_GRID_ALIGN_CENTER, 2, 1);

  lv_obj_t *cell_lbl = lv_obj_create(cont);
  lv_obj_remove_style_all(cell_lbl);
  lv_obj_set_style_bg_opa(cell_lbl, LV_OPA_TRANSP, 0);
  lv_obj_set_size(cell_lbl, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

  lv_obj_set_grid_cell(
    cell_lbl,
    LV_GRID_ALIGN_CENTER, 1, 1,
    LV_GRID_ALIGN_CENTER, 2, 1);

  //lv_obj_add_flag(pitch_ui.car, LV_OBJ_FLAG_HIDDEN);
  //lv_obj_add_flag(roll_ui.car,  LV_OBJ_FLAG_HIDDEN);

  attitude_set_value(&pitch_ui, 20.0f);  // +20°
  attitude_set_value(&roll_ui, -15.0f);  // -15°
}

void loop() {
  static uint32_t last = millis();
  uint32_t now = millis();
  lv_tick_inc(now - last);
  last = now;

  lv_timer_handler();

  // tpms::TpmsPacket p;
  // if (tpms::tryReadTpms(p)) {
  //   Serial.print("seq=");
  //   Serial.print(p.sequence);
  //   Serial.print(" id=");
  //   Serial.print(p.sensorId);
  //   Serial.print(" pressure=");
  //   Serial.print(p.pressure);
  //   Serial.print(" temp=");
  //   Serial.println(p.temp);
  // }
  delay(5);
}