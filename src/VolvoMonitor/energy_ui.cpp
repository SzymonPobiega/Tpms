#include "energy_ui.hpp"
#include <string.h>
#include <stdio.h>

namespace CamperUI {

static Callbacks g_cb;

// --- UI handles ---
static lv_obj_t* g_lblSoc = nullptr;
static lv_obj_t* g_lblAh  = nullptr;
static lv_obj_t* g_lblChg = nullptr;

static lv_obj_t* g_lightBtn[2][4] = {{nullptr}};
static lv_obj_t* g_lightLbl[2][4] = {{nullptr}};
static bool      g_lightState[2][4] = {{false}};

static lv_obj_t* g_hatchStatus = nullptr;
static lv_obj_t* g_roofStatus  = nullptr;

// --- Runtime sizing tuned for 800x480 ---
static lv_coord_t UI_PAD        = 8;
static lv_coord_t UI_GAP        = 8;
static lv_coord_t UI_RADIUS     = 10;
static lv_coord_t UI_STATUS_H   = 64;
static lv_coord_t UI_MECH_H     = 96;
static lv_coord_t UI_PILL_H     = 42;
static lv_coord_t UI_ALL_OFF_W  = 110;
static lv_coord_t UI_ALL_OFF_H  = 42;
static lv_coord_t UI_TILE_H     = 92;
static lv_coord_t UI_ACT_BTN_H  = 76;
static lv_coord_t UI_CARD_PAD   = 8;
static lv_coord_t UI_BTN_PAD    = 8;
static lv_coord_t UI_PILL_PAD_H = 10;
static lv_coord_t UI_PILL_PAD_V = 6;

// Fonts chosen for smaller screen
static const lv_font_t* FONT_METRIC   = &lv_font_montserrat_18;
static const lv_font_t* FONT_TILE     = &lv_font_montserrat_20;
static const lv_font_t* FONT_ACTION   = &lv_font_montserrat_20;
static const lv_font_t* FONT_ARROW    = &lv_font_montserrat_32;

// --- Style helpers ---
static lv_style_t style_card;
static lv_style_t style_pill;
static lv_style_t style_btn;
static lv_style_t style_btn_on;
static lv_style_t style_btn_off;
static lv_style_t style_small_pill_on;
static lv_style_t style_small_pill_off;
static lv_style_t style_act_btn_idle;
static lv_style_t style_act_btn_pressed;

static bool styles_inited = false;

static void init_styles()
{
    if (styles_inited) return;
    styles_inited = true;

    lv_style_init(&style_card);
    lv_style_set_radius(&style_card, UI_RADIUS);
    lv_style_set_pad_all(&style_card, UI_CARD_PAD);
    lv_style_set_border_width(&style_card, 1);
    lv_style_set_border_color(&style_card, lv_color_hex(0x404040));
    lv_style_set_bg_color(&style_card, lv_color_hex(0x1C1C1C));
    lv_style_set_bg_opa(&style_card, LV_OPA_COVER);

    lv_style_init(&style_pill);
    lv_style_set_radius(&style_pill, UI_RADIUS + 4);
    lv_style_set_pad_hor(&style_pill, UI_PILL_PAD_H);
    lv_style_set_pad_ver(&style_pill, UI_PILL_PAD_V);
    lv_style_set_border_width(&style_pill, 1);
    lv_style_set_border_color(&style_pill, lv_color_hex(0x505050));
    lv_style_set_bg_color(&style_pill, lv_color_hex(0x101010));
    lv_style_set_bg_opa(&style_pill, LV_OPA_COVER);

    lv_style_init(&style_btn);
    lv_style_set_radius(&style_btn, UI_RADIUS);
    lv_style_set_pad_all(&style_btn, UI_BTN_PAD);
    lv_style_set_border_width(&style_btn, 1);
    lv_style_set_border_color(&style_btn, lv_color_hex(0x555555));
    lv_style_set_text_color(&style_btn, lv_color_hex(0xFFFFFF));

    lv_style_init(&style_btn_on);
    lv_style_set_bg_color(&style_btn_on, lv_color_hex(0x1F6D2A));
    lv_style_set_bg_opa(&style_btn_on, LV_OPA_COVER);

    lv_style_init(&style_btn_off);
    lv_style_set_bg_color(&style_btn_off, lv_color_hex(0x2A2A2A));
    lv_style_set_bg_opa(&style_btn_off, LV_OPA_COVER);

    lv_style_init(&style_small_pill_on);
    lv_style_set_radius(&style_small_pill_on, UI_RADIUS + 2);
    lv_style_set_pad_hor(&style_small_pill_on, 8);
    lv_style_set_pad_ver(&style_small_pill_on, 5);
    lv_style_set_bg_color(&style_small_pill_on, lv_color_hex(0x2AAE3A));
    lv_style_set_bg_opa(&style_small_pill_on, LV_OPA_COVER);

    lv_style_init(&style_small_pill_off);
    lv_style_set_radius(&style_small_pill_off, UI_RADIUS + 2);
    lv_style_set_pad_hor(&style_small_pill_off, 8);
    lv_style_set_pad_ver(&style_small_pill_off, 5);
    lv_style_set_bg_color(&style_small_pill_off, lv_color_hex(0x6A6A6A));
    lv_style_set_bg_opa(&style_small_pill_off, LV_OPA_COVER);

    lv_style_init(&style_act_btn_idle);
    lv_style_set_radius(&style_act_btn_idle, UI_RADIUS);
    lv_style_set_bg_color(&style_act_btn_idle, lv_color_hex(0x2A2A2A));
    lv_style_set_bg_opa(&style_act_btn_idle, LV_OPA_COVER);
    lv_style_set_border_width(&style_act_btn_idle, 1);
    lv_style_set_border_color(&style_act_btn_idle, lv_color_hex(0x555555));
    lv_style_set_text_color(&style_act_btn_idle, lv_color_hex(0xFFFFFF));
    lv_style_set_pad_all(&style_act_btn_idle, UI_BTN_PAD);

    lv_style_init(&style_act_btn_pressed);
    lv_style_set_radius(&style_act_btn_pressed, UI_RADIUS);
    lv_style_set_bg_color(&style_act_btn_pressed, lv_color_hex(0x1F6D2A));
    lv_style_set_bg_opa(&style_act_btn_pressed, LV_OPA_COVER);
    lv_style_set_border_width(&style_act_btn_pressed, 1);
    lv_style_set_border_color(&style_act_btn_pressed, lv_color_hex(0x555555));
    lv_style_set_text_color(&style_act_btn_pressed, lv_color_hex(0xFFFFFF));
    lv_style_set_pad_all(&style_act_btn_pressed, UI_BTN_PAD);
}

static void apply_light_visual(CamperUI::LightGroup group, uint8_t idx)
{
    const uint8_t g = (group == LightGroup::Interior) ? 0 : 1;
    bool on = g_lightState[g][idx];

    lv_obj_t* btn = g_lightBtn[g][idx];
    if (!btn) return;

    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(btn,
                              on ? lv_color_hex(0x1F6D2A) : lv_color_hex(0x2A2A2A),
                              0);

    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x555555), 0);
    lv_obj_set_style_radius(btn, UI_RADIUS, 0);

    if (g_lightLbl[g][idx]) {
        lv_obj_set_style_text_color(g_lightLbl[g][idx],
                                    on ? lv_color_hex(0xFFFFFF) : lv_color_hex(0xDADADA),
                                    0);
    }
}

static void light_btn_event_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    uintptr_t packed = (uintptr_t)lv_event_get_user_data(e);
    uint8_t g = (packed & 0x1) ? 1 : 0;
    uint8_t idx = (uint8_t)((packed >> 8) & 0xFF);
    if (idx > 3) return;

    g_lightState[g][idx] = !g_lightState[g][idx];
    apply_light_visual(g ? LightGroup::Exterior : LightGroup::Interior, idx);

    if (g_cb.onLightChanged) {
        g_cb.onLightChanged(g ? LightGroup::Exterior : LightGroup::Interior, idx, g_lightState[g][idx]);
    }
}

static void all_off_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_LONG_PRESSED) {
        for (uint8_t g = 0; g < 2; g++) {
            for (uint8_t i = 0; i < 4; i++) {
                g_lightState[g][i] = false;
                apply_light_visual(g ? LightGroup::Exterior : LightGroup::Interior, i);
                if (g_cb.onLightChanged) {
                    g_cb.onLightChanged(g ? LightGroup::Exterior : LightGroup::Interior, i, false);
                }
            }
        }
        if (g_cb.onAllOff) g_cb.onAllOff();
    }
}

static void actuator_btn_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* btn = lv_event_get_target(e);

    uintptr_t packed = (uintptr_t)lv_event_get_user_data(e);
    uint8_t dir = (packed & 0x1) ? 1 : 0;
    uint8_t act = (uint8_t)((packed >> 8) & 0xFF);
    if (act > 1) return;

    Actuator a = act ? Actuator::Roof : Actuator::Hatch;
    Direction d = dir ? Direction::Down : Direction::Up;

    if (code == LV_EVENT_PRESSED) {
        lv_obj_remove_style_all(btn);
        lv_obj_add_style(btn, &style_act_btn_pressed, 0);

        if (g_cb.onActuatorPressed) g_cb.onActuatorPressed(a, d);
        return;
    }

    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        lv_obj_remove_style_all(btn);
        lv_obj_add_style(btn, &style_act_btn_idle, 0);

        if (g_cb.onActuatorReleased) g_cb.onActuatorReleased(a, d);
        return;
    }
}

static void make_actuator_pair(lv_obj_t* card, Actuator act)
{
    const lv_coord_t GAP = UI_GAP;

    lv_obj_t* grid = lv_obj_create(card);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(grid, LV_PCT(100), UI_ACT_BTN_H);

    lv_obj_set_style_pad_column(grid, GAP, 0);
    lv_obj_set_style_pad_row(grid, 0, 0);

    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);

    auto make_one = [&](const char* txt, uint8_t dirBit, lv_coord_t col) {
        lv_obj_t* b = lv_btn_create(grid);
        lv_obj_remove_style_all(b);
        lv_obj_add_style(b, &style_act_btn_idle, 0);

        lv_obj_set_grid_cell(b,
                             LV_GRID_ALIGN_STRETCH, col, 1,
                             LV_GRID_ALIGN_STRETCH, 0, 1);

        lv_obj_add_event_cb(b, actuator_btn_event_cb, LV_EVENT_PRESSED,
                            (void*)(uintptr_t)(((uintptr_t)act << 8) | dirBit));
        lv_obj_add_event_cb(b, actuator_btn_event_cb, LV_EVENT_RELEASED,
                            (void*)(uintptr_t)(((uintptr_t)act << 8) | dirBit));
        lv_obj_add_event_cb(b, actuator_btn_event_cb, LV_EVENT_PRESS_LOST,
                            (void*)(uintptr_t)(((uintptr_t)act << 8) | dirBit));

        lv_obj_t* l = lv_label_create(b);
        lv_label_set_text(l, txt);
        lv_obj_set_style_text_font(l, FONT_ARROW, 0);
        lv_obj_center(l);
    };

    make_one(LV_SYMBOL_UP, 0, 0);
    make_one(LV_SYMBOL_DOWN, 1, 1);
}

static lv_obj_t* make_metric_pill(lv_obj_t* parent, const char* initial_text, lv_obj_t** out_label)
{
    lv_obj_t* pill = lv_obj_create(parent);
    lv_obj_add_style(pill, &style_pill, 0);
    lv_obj_set_height(pill, UI_PILL_H);

    lv_obj_t* lbl = lv_label_create(pill);
    lv_label_set_text(lbl, initial_text);
    lv_obj_set_style_text_font(lbl, FONT_METRIC, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);   // <- add this
    lv_obj_center(lbl);

    if (out_label) *out_label = lbl;
    return pill;
}

static lv_obj_t* make_light_button(lv_obj_t* parent, const char* name,
                                   CamperUI::LightGroup group, uint8_t idx)
{
    const uint8_t g = (group == CamperUI::LightGroup::Interior) ? 0 : 1;

    lv_obj_t* btn = lv_btn_create(parent);
    lv_obj_set_size(btn, LV_PCT(100), LV_PCT(100));
    lv_obj_add_event_cb(btn, light_btn_event_cb, LV_EVENT_CLICKED,
                        (void*)(uintptr_t)(((uintptr_t)idx << 8) | (uintptr_t)g));

    lv_obj_set_style_pad_all(btn, UI_BTN_PAD, 0);

    lv_obj_t* lbl = lv_label_create(btn);
    lv_label_set_text(lbl, name);
    lv_obj_set_style_text_font(lbl, FONT_TILE, 0);
    lv_obj_center(lbl);

    g_lightBtn[g][idx] = btn;
    g_lightLbl[g][idx] = lbl;

    apply_light_visual(group, idx);
    return btn;
}

static lv_obj_t* make_card(lv_obj_t* parent)
{
    lv_obj_t* card = lv_obj_create(parent);

    lv_obj_add_style(card, &style_card, 0);

    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);

    lv_obj_set_style_pad_all(card, UI_CARD_PAD, 0);

    return card;
}

void init(lv_obj_t* parent, lv_coord_t width, lv_coord_t height, const Callbacks& cb)
{
    g_cb = cb;

    // Values tuned for 800x480 class display
    UI_PAD        = 8;
    UI_GAP        = 8;
    UI_RADIUS     = 10;
    UI_STATUS_H   = 64;
    UI_MECH_H     = 96;
    UI_PILL_H     = 42;
    UI_ALL_OFF_W  = 110;
    UI_ALL_OFF_H  = 42;
    UI_TILE_H     = 92;
    UI_ACT_BTN_H  = 76;
    UI_CARD_PAD   = 8;
    UI_BTN_PAD    = 8;
    UI_PILL_PAD_H = 10;
    UI_PILL_PAD_V = 6;

    init_styles();

    lv_obj_t* root = parent;
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(root, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_set_style_pad_all(root, UI_PAD, 0);
    lv_obj_set_style_pad_row(root, UI_GAP, 0);

    // STATUS BAR
    lv_obj_t* status = lv_obj_create(root);
    lv_obj_remove_style_all(status);
    lv_obj_set_size(status, LV_PCT(100), UI_STATUS_H);
    lv_obj_set_style_radius(status, UI_RADIUS, 0);
    lv_obj_set_style_bg_color(status, lv_color_hex(0x0F0F0F), 0);
    lv_obj_set_style_bg_opa(status, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(status, UI_PAD, 0);

    lv_obj_set_flex_flow(status, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(status, UI_GAP, 0);

    lv_obj_t* metrics = lv_obj_create(status);
    lv_obj_remove_style_all(metrics);
    lv_obj_set_flex_flow(metrics, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(metrics, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(metrics, UI_GAP, 0);
    lv_obj_set_flex_grow(metrics, 1);

    make_metric_pill(metrics, "SOC: 0%", &g_lblSoc);
    make_metric_pill(metrics, "Ah: 0.0", &g_lblAh);
    make_metric_pill(metrics, "CHG: +0.0A", &g_lblChg);

    lv_obj_t* btnAllOff = lv_btn_create(status);
    lv_obj_set_size(btnAllOff, UI_ALL_OFF_W, UI_ALL_OFF_H);
    lv_obj_set_style_radius(btnAllOff, UI_RADIUS, 0);
    lv_obj_set_style_bg_color(btnAllOff, lv_color_hex(0xB00020), 0);
    lv_obj_set_style_bg_opa(btnAllOff, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(btnAllOff, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_anim_time(btnAllOff, 80, LV_STATE_PRESSED);
    lv_obj_add_event_cb(btnAllOff, all_off_event_cb, LV_EVENT_LONG_PRESSED, nullptr);

    lv_obj_t* lblAll = lv_label_create(btnAllOff);
    lv_label_set_text(lblAll, "OFF");
    lv_obj_set_style_text_font(lblAll, FONT_ACTION, 0);
    lv_obj_center(lblAll);

    // GEOMETRY
    lv_coord_t content_w = width - 2 * UI_PAD;
    lv_coord_t lights_h  = height - 2 * UI_PAD - 2 * UI_GAP - UI_STATUS_H - UI_MECH_H;
    if (lights_h < 150) lights_h = 150;

    // LIGHTS AREA
    lv_obj_t* lightsRow = lv_obj_create(root);
    lv_obj_set_style_bg_opa(lightsRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(lightsRow, 0, 0);
    lv_obj_set_size(lightsRow, content_w, lights_h);
    lv_obj_clear_flag(lightsRow, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_flex_flow(lightsRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(lightsRow,
                          LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(lightsRow, 0, 0);
    lv_obj_set_style_pad_column(lightsRow, UI_GAP, 0);

    lv_obj_t* cardIn  = make_card(lightsRow);
    lv_obj_t* cardOut = make_card(lightsRow);

    lv_coord_t card_w = (content_w - UI_GAP) / 2;
    lv_obj_set_size(cardIn,  card_w, lights_h);
    lv_obj_set_size(cardOut, card_w, lights_h);

    lv_obj_clear_flag(cardIn,  LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(cardOut, LV_OBJ_FLAG_SCROLLABLE);

    auto make_grid = [](lv_obj_t* card) -> lv_obj_t* {
        lv_obj_t* grid = lv_obj_create(card);
        lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(grid, 0, 0);
        lv_obj_set_size(grid, LV_PCT(100), LV_PCT(100));
        lv_obj_set_flex_grow(grid, 1);
        lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_pad_row(grid, UI_GAP, 0);
        lv_obj_set_style_pad_column(grid, UI_GAP, 0);

        static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
        static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
        lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
        return grid;
    };

    lv_obj_t* gridIn  = make_grid(cardIn);
    lv_obj_t* gridOut = make_grid(cardOut);

    lv_obj_clear_flag(gridIn,  LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(gridOut, LV_OBJ_FLAG_SCROLLABLE);

    const char* inNames[4]  = {"Cab", "Front", "Living", "Rear"};
    const char* outNames[4] = {"Front", "Left", "Right", "Rear"};

    for (uint8_t i = 0; i < 4; i++) {
        lv_obj_t* b = make_light_button(gridIn, inNames[i], LightGroup::Interior, i);
        lv_obj_set_grid_cell(b,
                             LV_GRID_ALIGN_STRETCH, (i % 2), 1,
                             LV_GRID_ALIGN_STRETCH, (i / 2), 1);
        lv_obj_set_height(b, UI_TILE_H);
    }

    for (uint8_t i = 0; i < 4; i++) {
        lv_obj_t* b = make_light_button(gridOut, outNames[i], LightGroup::Exterior, i);
        lv_obj_set_grid_cell(b,
                             LV_GRID_ALIGN_STRETCH, (i % 2), 1,
                             LV_GRID_ALIGN_STRETCH, (i / 2), 1);
        lv_obj_set_height(b, UI_TILE_H);
    }

    // MECHANICS AREA
    lv_obj_t* mechRow = lv_obj_create(root);
    lv_obj_set_style_bg_opa(mechRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(mechRow, 0, 0);
    lv_obj_set_size(mechRow, content_w, UI_MECH_H);
    lv_obj_clear_flag(mechRow, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_flex_flow(mechRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(mechRow,
                          LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(mechRow, 0, 0);
    lv_obj_set_style_pad_column(mechRow, UI_GAP, 0);

    lv_obj_t* cardHatch = make_card(mechRow);
    lv_obj_t* cardRoof  = make_card(mechRow);

    lv_obj_set_size(cardHatch, card_w, UI_MECH_H);
    lv_obj_set_size(cardRoof,  card_w, UI_MECH_H);

    lv_obj_clear_flag(cardHatch, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(cardRoof,  LV_OBJ_FLAG_SCROLLABLE);

    make_actuator_pair(cardHatch, Actuator::Hatch);
    make_actuator_pair(cardRoof,  Actuator::Roof);

    setSocPercent(77);
    setRemainingAh(0.0f);
    setChargeCurrentA(0.0f);
}

void setSocPercent(uint8_t soc)
{
    if (!g_lblSoc) return;
    if (soc > 100) soc = 100;
    lv_label_set_text_fmt(g_lblSoc, "SOC: %u%%", (unsigned)soc);
}

void setRemainingAh(float ah)
{
    if (!g_lblAh) return;
    char buf[32];
    snprintf(buf, sizeof(buf), "Ah: %.1f", ah);
    lv_label_set_text(g_lblAh, buf);
}

void setChargeCurrentA(float a)
{
    if (!g_lblChg) return;
    char buf[32];
    snprintf(buf, sizeof(buf), "CHG: +%.1fA", a);
    lv_label_set_text(g_lblChg, buf);
}

void setLightState(LightGroup group, uint8_t index0to3, bool on)
{
    if (index0to3 > 3) return;
    uint8_t g = (group == LightGroup::Interior) ? 0 : 1;
    g_lightState[g][index0to3] = on;
    if (g_lightBtn[g][index0to3]) {
        apply_light_visual(group, index0to3);
    }
}

} // namespace CamperUI