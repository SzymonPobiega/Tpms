#include "camera_ui.hpp"

namespace {

struct CameraUI {
    lv_obj_t *btn[4];
    lv_obj_t *lbl[4];
};

struct BtnCtx {
    CameraUI *ui;
    uint8_t idx;
};

static CameraCallback g_callback = nullptr;

static lv_style_t style_btn_base;
static lv_style_t style_btn_checked;
static lv_style_t style_btn_pressed;
static bool styles_inited = false;

static constexpr lv_coord_t UI_PAD    = 8;
static constexpr lv_coord_t UI_GAP    = 8;
static constexpr lv_coord_t UI_RADIUS = 10;

static void init_styles_once()
{
    if (styles_inited) return;
    styles_inited = true;

    // Default dark tile, similar to CamperUI light buttons
    lv_style_init(&style_btn_base);
    lv_style_set_radius(&style_btn_base, UI_RADIUS);
    lv_style_set_border_width(&style_btn_base, 1);
    lv_style_set_border_color(&style_btn_base, lv_color_hex(0x555555));
    lv_style_set_bg_opa(&style_btn_base, LV_OPA_COVER);
    lv_style_set_bg_color(&style_btn_base, lv_color_hex(0x2A2A2A));
    lv_style_set_text_color(&style_btn_base, lv_color_hex(0xFFFFFF));
    lv_style_set_pad_all(&style_btn_base, UI_PAD);

    // Selected = green like active light button
    lv_style_init(&style_btn_checked);
    lv_style_set_radius(&style_btn_checked, UI_RADIUS);
    lv_style_set_border_width(&style_btn_checked, 1);
    lv_style_set_border_color(&style_btn_checked, lv_color_hex(0x555555));
    lv_style_set_bg_opa(&style_btn_checked, LV_OPA_COVER);
    lv_style_set_bg_color(&style_btn_checked, lv_color_hex(0x1F6D2A));
    lv_style_set_text_color(&style_btn_checked, lv_color_hex(0xFFFFFF));

    // Pressed = slightly darker feedback
    lv_style_init(&style_btn_pressed);
    lv_style_set_radius(&style_btn_pressed, UI_RADIUS);
    lv_style_set_bg_opa(&style_btn_pressed, LV_OPA_COVER);
    lv_style_set_bg_color(&style_btn_pressed, lv_color_hex(0x3A3A3A));
    lv_style_set_border_width(&style_btn_pressed, 1);
    lv_style_set_border_color(&style_btn_pressed, lv_color_hex(0x666666));
    lv_style_set_text_color(&style_btn_pressed, lv_color_hex(0xFFFFFF));
}

static void camera_set_selected(CameraUI *ui, uint8_t sel)
{
    for (uint8_t i = 0; i < 4; i++) {
        if (!ui->btn[i]) continue;

        if (i == sel) {
            lv_obj_add_state(ui->btn[i], LV_STATE_CHECKED);
            if (ui->lbl[i]) {
                lv_obj_set_style_text_color(ui->lbl[i], lv_color_hex(0xFFFFFF), 0);
            }
        } else {
            lv_obj_clear_state(ui->btn[i], LV_STATE_CHECKED);
            if (ui->lbl[i]) {
                lv_obj_set_style_text_color(ui->lbl[i], lv_color_hex(0xDADADA), 0);
            }
        }
    }
}

static void camera_btn_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    BtnCtx *ctx = (BtnCtx*)lv_event_get_user_data(e);
    if (!ctx || !ctx->ui) return;

    camera_set_selected(ctx->ui, ctx->idx);

    if (g_callback) {
        g_callback(ctx->idx);
    }
}

} // namespace

void camera_ui_set_callback(CameraCallback cb)
{
    g_callback = cb;
}

void setup_camera_tab(lv_obj_t *parent, int /*disp_w*/, int /*disp_h*/)
{
    init_styles_once();

    lv_obj_set_layout(parent, LV_LAYOUT_GRID);
    lv_obj_set_style_pad_all(parent, UI_PAD, 0);
    lv_obj_set_style_pad_row(parent, UI_GAP, 0);
    lv_obj_set_style_pad_column(parent, UI_GAP, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(parent, 0, 0);

    static lv_coord_t col_dsc[] = {
        LV_GRID_FR(1), LV_GRID_FR(1),
        LV_GRID_TEMPLATE_LAST
    };

    static lv_coord_t row_dsc[] = {
        LV_GRID_FR(1), LV_GRID_FR(1),
        LV_GRID_TEMPLATE_LAST
    };

    lv_obj_set_grid_dsc_array(parent, col_dsc, row_dsc);

    CameraUI *ui = (CameraUI*)lv_mem_alloc(sizeof(CameraUI));
    lv_memset_00(ui, sizeof(CameraUI));

    const char *labels[4] = {
        "Rear high",
        "Rear low",
        "Front high",
        "Front low"
    };

    for (uint8_t i = 0; i < 4; i++) {
        lv_obj_t *btn = lv_btn_create(parent);
        lv_obj_remove_style_all(btn);

        lv_obj_add_style(btn, &style_btn_base,    LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(btn, &style_btn_checked, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_add_style(btn, &style_btn_pressed, LV_PART_MAIN | LV_STATE_PRESSED);

        lv_obj_set_size(btn, LV_PCT(100), LV_PCT(100));
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

        uint8_t col = i % 2;
        uint8_t row = i / 2;

        lv_obj_set_grid_cell(btn,
            LV_GRID_ALIGN_STRETCH, col, 1,
            LV_GRID_ALIGN_STRETCH, row, 1);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, labels[i]);
        lv_obj_set_width(lbl, LV_PCT(100));
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_26, 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xDADADA), 0);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
        lv_obj_center(lbl);

        BtnCtx *ctx = (BtnCtx*)lv_mem_alloc(sizeof(BtnCtx));
        ctx->ui = ui;
        ctx->idx = i;

        lv_obj_add_event_cb(btn, camera_btn_event_cb, LV_EVENT_CLICKED, ctx);

        ui->btn[i] = btn;
        ui->lbl[i] = lbl;
    }

    // Default selection
    camera_set_selected(ui, 0);
}