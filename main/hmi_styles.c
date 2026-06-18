#include "hmi.h"

// Прототип локальной функции, чтобы структура увидела её адрес
static void init_styles(void);

// Создаем глобальный экземпляр структуры и сразу привязываем функцию к полю .init
hmi_styles_t hmi_styles = {
    .init = init_styles
};

static void init_styles(void)
{
    lv_style_init(&hmi_styles.st_root);
    lv_style_set_bg_color(&hmi_styles.st_root, C_BG);
    lv_style_set_bg_opa(&hmi_styles.st_root, LV_OPA_COVER);
    lv_style_set_text_color(&hmi_styles.st_root, C_TEXT);

    lv_style_init(&hmi_styles.st_topbar);
    lv_style_set_bg_color(&hmi_styles.st_topbar, C_TOP);
    lv_style_set_bg_opa(&hmi_styles.st_topbar, LV_OPA_COVER);
    lv_style_set_border_width(&hmi_styles.st_topbar, 2);
    lv_style_set_border_color(&hmi_styles.st_topbar, lv_color_hex(0x1F2B36));
    lv_style_set_pad_all(&hmi_styles.st_topbar, 16);

    lv_style_init(&hmi_styles.st_panel);
    lv_style_set_bg_color(&hmi_styles.st_panel, C_PANEL);
    lv_style_set_bg_opa(&hmi_styles.st_panel, LV_OPA_COVER);
    lv_style_set_border_width(&hmi_styles.st_panel, 1);
    lv_style_set_border_color(&hmi_styles.st_panel, C_BORDER);
    lv_style_set_radius(&hmi_styles.st_panel, 10);
    lv_style_set_pad_all(&hmi_styles.st_panel, 18);
    lv_style_set_shadow_width(&hmi_styles.st_panel, 18);
    lv_style_set_shadow_opa(&hmi_styles.st_panel, LV_OPA_40);

    lv_style_init(&hmi_styles.st_panel_clickable);
    lv_style_set_border_color(&hmi_styles.st_panel_clickable, C_BLUE);

    lv_style_init(&hmi_styles.st_text);
    lv_style_set_text_color(&hmi_styles.st_text, C_TEXT);
    lv_style_set_text_font(&hmi_styles.st_text, lv_font_20);

    lv_style_init(&hmi_styles.st_muted);
    lv_style_set_text_color(&hmi_styles.st_muted, C_MUTED);
    lv_style_set_text_font(&hmi_styles.st_muted, lv_font_22);

    lv_style_init(&hmi_styles.st_big_green);
    lv_style_set_text_color(&hmi_styles.st_big_green, C_GREEN);
    lv_style_set_text_font(&hmi_styles.st_big_green, lv_font_48);

    lv_style_init(&hmi_styles.st_big_blue);
    lv_style_set_text_color(&hmi_styles.st_big_blue, C_BLUE);
    lv_style_set_text_font(&hmi_styles.st_big_blue, lv_font_48);

    lv_style_init(&hmi_styles.st_big_orange);
    lv_style_set_text_color(&hmi_styles.st_big_orange, C_ORANGE);
    lv_style_set_text_font(&hmi_styles.st_big_orange, lv_font_48);

    lv_style_init(&hmi_styles.st_btn_dark);
    lv_style_set_bg_color(&hmi_styles.st_btn_dark, C_PANEL_2);
    lv_style_set_bg_opa(&hmi_styles.st_btn_dark, LV_OPA_COVER);
    lv_style_set_border_width(&hmi_styles.st_btn_dark, 1);
    lv_style_set_border_color(&hmi_styles.st_btn_dark, C_BORDER_LT);
    lv_style_set_radius(&hmi_styles.st_btn_dark, 8);
    lv_style_set_pad_all(&hmi_styles.st_btn_dark, 12);

    lv_style_init(&hmi_styles.st_btn_green);
    lv_style_set_bg_color(&hmi_styles.st_btn_green, lv_color_hex(0x1F9A3A));
    lv_style_set_bg_grad_color(&hmi_styles.st_btn_green, lv_color_hex(0x39C95A));
    lv_style_set_bg_grad_dir(&hmi_styles.st_btn_green, LV_GRAD_DIR_VER);
    lv_style_set_radius(&hmi_styles.st_btn_green, 10);

    lv_style_init(&hmi_styles.st_btn_red);
    lv_style_set_bg_color(&hmi_styles.st_btn_red, lv_color_hex(0xB91515));
    lv_style_set_bg_grad_color(&hmi_styles.st_btn_red, lv_color_hex(0xF33434));
    lv_style_set_bg_grad_dir(&hmi_styles.st_btn_red, LV_GRAD_DIR_VER);
    lv_style_set_radius(&hmi_styles.st_btn_red, 10);

    lv_style_init(&hmi_styles.st_btn_blue);
    lv_style_set_bg_color(&hmi_styles.st_btn_blue, lv_color_hex(0x1662C8));
    lv_style_set_bg_grad_color(&hmi_styles.st_btn_blue, lv_color_hex(0x339CFF));
    lv_style_set_bg_grad_dir(&hmi_styles.st_btn_blue, LV_GRAD_DIR_VER);
    lv_style_set_radius(&hmi_styles.st_btn_blue, 8);

    lv_style_init(&hmi_styles.st_btn_gray);
    lv_style_set_bg_color(&hmi_styles.st_btn_gray, lv_color_hex(0x1C2732));
    lv_style_set_bg_grad_color(&hmi_styles.st_btn_gray, lv_color_hex(0x334250));
    lv_style_set_bg_grad_dir(&hmi_styles.st_btn_gray, LV_GRAD_DIR_VER);
}

__attribute__((constructor)) static void register_styles_api(void) {
    hmi_styles.init = init_styles;
}