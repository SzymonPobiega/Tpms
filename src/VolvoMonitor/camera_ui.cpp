#include "camera_ui.hpp"

namespace {

struct CameraUI {
    lv_obj_t *btn[4];
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



static void init_styles_once()
{
    if(styles_inited) return;
    styles_inited = true;

    // Default grey button
    lv_style_init(&style_btn_base);
    lv_style_set_radius(&style_btn_base, 0);
    lv_style_set_border_width(&style_btn_base, 0);
    lv_style_set_bg_opa(&style_btn_base, LV_OPA_COVER);
    lv_style_set_bg_color(&style_btn_base, lv_color_make(140,140,140));
    lv_style_set_text_color(&style_btn_base, lv_color_white());
    lv_style_set_text_font(&style_btn_base, &lv_font_montserrat_40);

    // Selected (green)
    lv_style_init(&style_btn_checked);
    lv_style_set_radius(&style_btn_checked, 0);
    lv_style_set_border_width(&style_btn_checked, 0);
    lv_style_set_bg_opa(&style_btn_checked, LV_OPA_COVER);
    lv_style_set_bg_color(&style_btn_checked, lv_color_make(0,180,0));
    lv_style_set_text_color(&style_btn_checked, lv_color_white());

    // Pressed (darker version)
    lv_style_init(&style_btn_pressed);
    lv_style_set_bg_opa(&style_btn_pressed, LV_OPA_COVER);
    lv_style_set_bg_color(&style_btn_pressed, lv_color_make(60,60,60));
}

static void camera_set_selected(CameraUI *ui, uint8_t sel)
{
    for(uint8_t i=0;i<4;i++)
    {
        if(!ui->btn[i]) continue;

        if(i == sel)
            lv_obj_add_state(ui->btn[i], LV_STATE_CHECKED);
        else
            lv_obj_clear_state(ui->btn[i], LV_STATE_CHECKED);
    }
}

static void camera_btn_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    BtnCtx *ctx = (BtnCtx*)lv_event_get_user_data(e);
    if(!ctx || !ctx->ui) return;

    camera_set_selected(ctx->ui, ctx->idx);

    if(g_callback)
        g_callback(ctx->idx);
}

}

void camera_ui_set_callback(CameraCallback cb)
{
    g_callback = cb;
}

void setup_camera_tab(lv_obj_t *parent, int /*disp_w*/, int /*disp_h*/)
{
    init_styles_once();

    lv_obj_set_layout(parent, LV_LAYOUT_GRID);
    lv_obj_set_style_pad_all(parent, 0, 0);
    lv_obj_set_style_pad_row(parent, 0, 0);
    lv_obj_set_style_pad_column(parent, 0, 0);

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

    for(uint8_t i=0;i<4;i++)
    {
        lv_obj_t *btn = lv_btn_create(parent);
        lv_obj_remove_style_all(btn);

        lv_obj_add_style(btn, &style_btn_base, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(btn, &style_btn_checked, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_add_style(btn, &style_btn_pressed, LV_PART_MAIN | LV_STATE_PRESSED);

        lv_obj_set_size(btn, lv_pct(100), lv_pct(100));
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

        uint8_t col = i % 2;
        uint8_t row = i / 2;

        lv_obj_set_grid_cell(btn,
            LV_GRID_ALIGN_STRETCH, col, 1,
            LV_GRID_ALIGN_STRETCH, row, 1);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, labels[i]);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(lbl);

        BtnCtx *ctx = (BtnCtx*)lv_mem_alloc(sizeof(BtnCtx));
        ctx->ui = ui;
        ctx->idx = i;

        lv_obj_add_event_cb(btn, camera_btn_event_cb, LV_EVENT_CLICKED, ctx);

        ui->btn[i] = btn;
    }

    // Default selection
    camera_set_selected(ui, 0);
}