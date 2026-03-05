#include "CamperUI.hpp"
#include <string.h>
#include <stdio.h>

namespace CamperUI {

static Callbacks g_cb;

// --- UI handles ---
static lv_obj_t* g_lblSoc = nullptr;
static lv_obj_t* g_lblAh  = nullptr;
static lv_obj_t* g_lblChg = nullptr;
static lv_obj_t* g_lblDis = nullptr;

static lv_obj_t* g_lightBtn[2][4] = {{nullptr}};
static lv_obj_t* g_lightLbl[2][4] = {{nullptr}};
static bool      g_lightState[2][4] = {{false}};

static lv_obj_t* g_hatchStatus = nullptr;
static lv_obj_t* g_roofStatus  = nullptr;

// --- Style helpers (minimal, LVGL-friendly) ---
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
    lv_style_set_radius(&style_card, 12);
    lv_style_set_pad_all(&style_card, 12);
    lv_style_set_border_width(&style_card, 1);
    lv_style_set_border_color(&style_card, lv_color_hex(0x404040));
    lv_style_set_bg_color(&style_card, lv_color_hex(0x1C1C1C));
    lv_style_set_bg_opa(&style_card, LV_OPA_COVER);

    lv_style_init(&style_pill);
    lv_style_set_radius(&style_pill, 18);
    lv_style_set_pad_hor(&style_pill, 14);
    lv_style_set_pad_ver(&style_pill, 10);
    lv_style_set_border_width(&style_pill, 1);
    lv_style_set_border_color(&style_pill, lv_color_hex(0x505050));
    lv_style_set_bg_color(&style_pill, lv_color_hex(0x101010));
    lv_style_set_bg_opa(&style_pill, LV_OPA_COVER);

    lv_style_init(&style_btn);
    lv_style_set_radius(&style_btn, 12);
    lv_style_set_pad_all(&style_btn, 10);
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
    lv_style_set_radius(&style_small_pill_on, 14);
    lv_style_set_pad_hor(&style_small_pill_on, 10);
    lv_style_set_pad_ver(&style_small_pill_on, 6);
    lv_style_set_bg_color(&style_small_pill_on, lv_color_hex(0x2AAE3A));
    lv_style_set_bg_opa(&style_small_pill_on, LV_OPA_COVER);

    lv_style_init(&style_small_pill_off);
    lv_style_set_radius(&style_small_pill_off, 14);
    lv_style_set_pad_hor(&style_small_pill_off, 10);
    lv_style_set_pad_ver(&style_small_pill_off, 6);
    lv_style_set_bg_color(&style_small_pill_off, lv_color_hex(0x6A6A6A));
    lv_style_set_bg_opa(&style_small_pill_off, LV_OPA_COVER);

    lv_style_init(&style_act_btn_idle);
    lv_style_set_radius(&style_act_btn_idle, 12);
    lv_style_set_bg_color(&style_act_btn_idle, lv_color_hex(0x2A2A2A));
    lv_style_set_bg_opa(&style_act_btn_idle, LV_OPA_COVER);
    lv_style_set_border_width(&style_act_btn_idle, 1);
    lv_style_set_border_color(&style_act_btn_idle, lv_color_hex(0x555555));
    lv_style_set_text_color(&style_act_btn_idle, lv_color_hex(0xFFFFFF));
    lv_style_set_pad_all(&style_act_btn_idle, 10);

    lv_style_init(&style_act_btn_pressed);
    lv_style_set_radius(&style_act_btn_pressed, 12);
    lv_style_set_bg_color(&style_act_btn_pressed, lv_color_hex(0x1F6D2A)); // green
    lv_style_set_bg_opa(&style_act_btn_pressed, LV_OPA_COVER);
    lv_style_set_border_width(&style_act_btn_pressed, 1);
    lv_style_set_border_color(&style_act_btn_pressed, lv_color_hex(0x555555));
    lv_style_set_text_color(&style_act_btn_pressed, lv_color_hex(0xFFFFFF));
    lv_style_set_pad_all(&style_act_btn_pressed, 10);
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
    lv_obj_set_style_radius(btn, 12, 0);

    // Optional: slightly brighter text when ON
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
    // pack: bit0 = group (0 int,1 ext), bits 8.. = index
    uint8_t g = (packed & 0x1) ? 1 : 0;
    uint8_t idx = (uint8_t)((packed >> 8) & 0xFF);
    if (idx > 3) return;

    g_lightState[g][idx] = !g_lightState[g][idx];
    apply_light_visual(g ? LightGroup::Exterior : LightGroup::Interior, idx);

    if (g_cb.onLightChanged) {
        g_cb.onLightChanged(g ? LightGroup::Exterior : LightGroup::Interior, idx, g_lightState[g][idx]);
    }
}

// Long press handler for ALL OFF
static void all_off_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_LONG_PRESSED) {
        // Turn off all lights visually + callback
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
    uint8_t dir = (packed & 0x1) ? 1 : 0;                 // 0 up, 1 down
    uint8_t act = (uint8_t)((packed >> 8) & 0xFF);        // 0 hatch, 1 roof
    if (act > 1) return;

    Actuator a = act ? Actuator::Roof : Actuator::Hatch;
    Direction d = dir ? Direction::Down : Direction::Up;

    if (code == LV_EVENT_PRESSED) {
        // Turn green while touched
        lv_obj_remove_style_all(btn);
        lv_obj_add_style(btn, &style_act_btn_pressed, 0);

        if (g_cb.onActuatorPressed) g_cb.onActuatorPressed(a, d);
        return;
    }

    // User lifted OR finger slid away OR touch aborted
    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        lv_obj_remove_style_all(btn);
        lv_obj_add_style(btn, &style_act_btn_idle, 0);

        if (g_cb.onActuatorReleased) g_cb.onActuatorReleased(a, d);
        return;
    }
}

static void make_actuator_pair(lv_obj_t* card, Actuator act)
{
    const lv_coord_t GAP   = 12;
    const lv_coord_t BTN_H = 120; // can adjust (e.g. 100-110)

    // Container inside the card for the two buttons
    lv_obj_t* grid = lv_obj_create(card);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);

    // Make it fill the card inner area; card already has padding=12 from style_card
    lv_obj_set_size(grid, LV_PCT(100), BTN_H);

    lv_obj_set_style_pad_column(grid, GAP, 0);
    lv_obj_set_style_pad_row(grid, 0, 0);

    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);

    auto make_one = [&](const char* txt, uint8_t dirBit, lv_coord_t col) {
        lv_obj_t* b = lv_btn_create(grid);
        lv_obj_remove_style_all(b);
        lv_obj_add_style(b, &style_act_btn_idle, 0);

        // Put into grid cell (half width automatically)
        lv_obj_set_grid_cell(b,
                             LV_GRID_ALIGN_STRETCH, col, 1,
                             LV_GRID_ALIGN_STRETCH, 0,   1);

        lv_obj_add_event_cb(b, actuator_btn_event_cb, LV_EVENT_PRESSED,
                            (void*)(uintptr_t)(((uintptr_t)act << 8) | dirBit));
        lv_obj_add_event_cb(b, actuator_btn_event_cb, LV_EVENT_RELEASED,
                            (void*)(uintptr_t)(((uintptr_t)act << 8) | dirBit));
        lv_obj_add_event_cb(b, actuator_btn_event_cb, LV_EVENT_PRESS_LOST,
                            (void*)(uintptr_t)(((uintptr_t)act << 8) | dirBit));

        lv_obj_t* l = lv_label_create(b);
        lv_label_set_text(l, txt);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_44, 0);
        lv_obj_center(l);
    };

    make_one(LV_SYMBOL_UP, 0, 0);
    make_one(LV_SYMBOL_DOWN, 1, 1);
}

static lv_obj_t* make_metric_pill(lv_obj_t* parent, const char* initial_text, lv_obj_t** out_label)
{
    lv_obj_t* pill = lv_obj_create(parent);
    lv_obj_add_style(pill, &style_pill, 0);
    lv_obj_set_height(pill, 56);

    lv_obj_t* lbl = lv_label_create(pill);
    lv_label_set_text(lbl, initial_text);
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

    // Make the whole thing feel like a tile
    lv_obj_set_style_pad_all(btn, 10, 0);

    lv_obj_t* lbl = lv_label_create(btn);
    lv_label_set_text(lbl, name);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_26, 0);
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

    // Layout inside the card
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);

    lv_obj_set_style_pad_all(card, 12, 0);

    return card;
}

void init(const Callbacks& cb)
{
    g_cb = cb;
    init_styles();

    lv_obj_t* scr = lv_scr_act();
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // Root layout: column
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // Padding and spacing
    lv_obj_set_style_pad_all(scr, 12, 0);
    lv_obj_set_style_pad_row(scr, 12, 0);

    // =========================
    // STATUS BAR (height ~80)
    // =========================
    lv_obj_t* status = lv_obj_create(scr);
    lv_obj_remove_style_all(status);
    lv_obj_set_size(status, LV_PCT(100), 80);
    lv_obj_set_style_radius(status, 12, 0);
    lv_obj_set_style_bg_color(status, lv_color_hex(0x0F0F0F), 0);
    lv_obj_set_style_bg_opa(status, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(status, 12, 0);

    lv_obj_set_flex_flow(status, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(status, 10, 0);

    // left metrics container
    lv_obj_t* metrics = lv_obj_create(status);
    lv_obj_remove_style_all(metrics);
    lv_obj_set_flex_flow(metrics, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(metrics, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(metrics, 10, 0);

    make_metric_pill(metrics, "SOC: 0%", &g_lblSoc);
    make_metric_pill(metrics, "Ah: 0.0", &g_lblAh);
    make_metric_pill(metrics, "CHG: +0.0A", &g_lblChg);
    make_metric_pill(metrics, "DIS: -0.0A", &g_lblDis);

    // All off button (long press)
    lv_obj_t* btnAllOff = lv_btn_create(status);
    lv_obj_set_size(btnAllOff, 260, 56);
    lv_obj_set_style_radius(btnAllOff, 12, 0);
    lv_obj_set_style_bg_color(btnAllOff, lv_color_hex(0xB00020), 0);
    lv_obj_set_style_bg_opa(btnAllOff, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(btnAllOff, lv_color_hex(0xFFFFFF), 0);

    // Make long press feel intentional
    lv_obj_set_style_anim_time(btnAllOff, 80, LV_STATE_PRESSED);
    lv_obj_add_event_cb(btnAllOff, all_off_event_cb, LV_EVENT_LONG_PRESSED, nullptr);

    lv_obj_t* lblAll = lv_label_create(btnAllOff);
    lv_label_set_text(lblAll, "ZGAŚ WSZYSTKO");
    lv_obj_set_style_text_font(lblAll, &lv_font_montserrat_24, 0);
    lv_obj_center(lblAll);

    // (Optional) you can globally adjust long-press time:
    // lv_indev_set_long_press_time(lv_indev_get_act(), 1200); // if you want per indev (depends on your port)

    // =========================
    // LIGHTS AREA (two cards)
    // =========================
    // Screen geometry
    static const lv_coord_t SCR_W = 1024;
    static const lv_coord_t SCR_H = 600;
    static const lv_coord_t PAD   = 12;
    static const lv_coord_t GAP   = 12;
    static const lv_coord_t STATUS_H = 80;
    static const lv_coord_t MECH_H   = 140;

    lv_coord_t content_w = SCR_W - 2 * PAD;
    lv_coord_t lights_h  = SCR_H - 2 * PAD - 2 * GAP - STATUS_H - MECH_H; // = 332

    lv_obj_t* lightsRow = lv_obj_create(scr);
    // Don't nuke styles; just make it visually transparent
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
    lv_obj_set_style_pad_column(lightsRow, GAP, 0);

    // Cards
    lv_obj_t* cardIn  = make_card(lightsRow);
    lv_obj_t* cardOut = make_card(lightsRow);

    lv_coord_t card_w = (content_w - GAP) / 2;
    lv_obj_set_size(cardIn,  card_w, lights_h);
    lv_obj_set_size(cardOut, card_w, lights_h);

    lv_obj_clear_flag(cardIn,  LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(cardOut, LV_OBJ_FLAG_SCROLLABLE);

    // Place content grids in cards
    // We'll create a grid container inside each card, below the title.
    auto make_grid = [](lv_obj_t* card) -> lv_obj_t* {
        lv_obj_t* grid = lv_obj_create(card);
        lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(grid, 0, 0);

        // Fill the card's inner area (respecting padding)
        lv_obj_set_size(grid, LV_PCT(100), LV_PCT(100));
        lv_obj_set_flex_grow(grid, 1);
        lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_set_style_pad_row(grid, 12, 0);
        lv_obj_set_style_pad_column(grid, 12, 0);

        static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
        static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
        lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
        return grid;
    };

    lv_obj_t* gridIn  = make_grid(cardIn);
    lv_obj_t* gridOut = make_grid(cardOut);

    lv_obj_clear_flag(gridIn,  LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(gridOut, LV_OBJ_FLAG_SCROLLABLE);

    // Interior lights 4
    // Rename freely: "Kuchnia", "Sypialnia", etc.
    const char* inNames[4]  = {"Kuchnia", "Sypialnia", "Korytarz", "Garaż"};
    const char* outNames[4] = {"Przód", "Tył", "Lewa", "Prawa"};

    for (uint8_t i = 0; i < 4; i++) {
        lv_obj_t* b = make_light_button(gridIn, inNames[i], LightGroup::Interior, i);
        lv_obj_set_grid_cell(b,
                             LV_GRID_ALIGN_STRETCH, (i % 2), 1,
                             LV_GRID_ALIGN_STRETCH, (i / 2), 1);
        lv_obj_set_height(b, 140);
    }
    for (uint8_t i = 0; i < 4; i++) {
        lv_obj_t* b = make_light_button(gridOut, outNames[i], LightGroup::Exterior, i);
        lv_obj_set_grid_cell(b,
                             LV_GRID_ALIGN_STRETCH, (i % 2), 1,
                             LV_GRID_ALIGN_STRETCH, (i / 2), 1);
        lv_obj_set_height(b, 140);
    }

    // =========================
    // MECHANICS AREA (two cards)
    // =========================
    lv_obj_t* mechRow = lv_obj_create(scr);

    // Don't nuke styles; just make transparent container
    lv_obj_set_style_bg_opa(mechRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(mechRow, 0, 0);

    lv_obj_set_size(mechRow, 1024 - 2*12, 140);           // content width, fixed height
    lv_obj_clear_flag(mechRow, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_flex_flow(mechRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(mechRow,
                          LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(mechRow, 0, 0);
    lv_obj_set_style_pad_column(mechRow, 12, 0);

    // Cards
    lv_obj_t* cardHatch = make_card(mechRow);
    lv_obj_t* cardRoof  = make_card(mechRow);

    // lv_coord_t content_w = 1024 - 2*12;
    // lv_coord_t card_w    = (content_w - 12) / 2;

    lv_obj_set_size(cardHatch, card_w, 140);
    lv_obj_set_size(cardRoof,  card_w, 140);

    lv_obj_clear_flag(cardHatch, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(cardRoof,  LV_OBJ_FLAG_SCROLLABLE);

    // IMPORTANT: Make the card itself hold the two buttons directly
    // The card has padding 12 via style_card; choose button height to fit inside.
    lv_coord_t btn_h = 140 - 24;            // card height minus padding top+bottom
    lv_coord_t inner_w = card_w - 24;       // card width minus padding left+right

    make_actuator_pair(cardHatch, Actuator::Hatch);
    make_actuator_pair(cardRoof,  Actuator::Roof);

    // Default metric values
    setSocPercent(77);
    setRemainingAh(12.0f);
    setChargeCurrentA(16.0f);
    setDischargeCurrentA(67.0f);
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
    lv_label_set_text_fmt(g_lblAh, "Ah: %.1f", (double)ah);
}

void setChargeCurrentA(float a)
{
    if (!g_lblChg) return;
    lv_label_set_text_fmt(g_lblChg, "CHG: +%.1fA", (double)a);
}

void setDischargeCurrentA(float a)
{
    if (!g_lblDis) return;
    // Display as negative (visual)
    lv_label_set_text_fmt(g_lblDis, "DIS: -%.1fA", (double)a);
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