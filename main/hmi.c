/*
 * LVGL HMI Main Menu
 * Основа переноса HTML-интерфейса промышленного HMI на LVGL.
 * Ориентир: LVGL 8.3+ / 9.x с классическим C API.
 *
 * Вызовите hmi_create() после lv_init(), инициализации дисплея и input device.
 */

#include "lvgl.h"
#include <stdio.h>
#include <stdbool.h>
#include <esp_log.h>
#include "esp_spiffs.h"
#include "src/misc/lv_fs_private.h"
#include "hmi.h"

#define HMI_SETPOINT_STEP 0.1f

#define LV_FONT_FMT_TXT_LARGE_USER_DATA 0

#define TAG "AI"

/* Состояние */
float actual_pressure = 7.8f;
float set_pressure = 10.0f;

bool auto_running = false;
bool manual_running = false;
hmi_page_t hmi_current_page = HMI_PAGE_AUTO;

/* Объекты */
static lv_obj_t *root;
lv_obj_t *screen_title;
lv_obj_t *clock_label;
static lv_obj_t *network_label;
static lv_obj_t *setpoint_modal;

void set_message(lv_obj_t *label, const char *text)
{
    char buf[160];
    snprintf(buf, sizeof(buf), LV_SYMBOL_OK "  %s", text);
    lv_label_set_text(label, buf);
}

void update_big_button(lv_obj_t *btn, bool running)
{
    lv_obj_t *icon = lv_obj_get_child(btn, 0);
    lv_obj_t *text = lv_obj_get_child(btn, 1);

    lv_label_set_text(icon, running ? LV_SYMBOL_STOP : LV_SYMBOL_PLAY);
    lv_label_set_text(text, running ? "СТОП" : "СТАРТ");

    lv_obj_remove_style(btn, ST_BTN_GREEN, 0);
    lv_obj_remove_style(btn, ST_BTN_RED, 0);
    lv_obj_add_style(btn, running ? ST_BTN_RED : ST_BTN_GREEN, 0);
}

lv_obj_t *label(lv_obj_t *parent, const char *text, const lv_style_t *style)
{
    lv_obj_t *obj = lv_label_create(parent);
    lv_label_set_text(obj, text);
    if (style)
        lv_obj_add_style(obj, style, 0);
    return obj;
}

lv_obj_t *panel(lv_obj_t *parent)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_add_style(obj, ST_PANEL, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

lv_obj_t *button(lv_obj_t *parent, const char *text, const lv_style_t *style)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_add_style(btn, style, 0);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *txt = label(btn, text, ST_TEXT);
    lv_obj_center(txt);
    return btn;
}

lv_obj_t *big_start_button(lv_obj_t *parent)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_add_style(btn, ST_BTN_GREEN, 0);
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(btn, 16, 0);

    lv_obj_t *icon = label(btn, LV_SYMBOL_PLAY, ST_TEXT);
    lv_obj_set_style_text_font(icon, lv_font_36, 0);

    lv_obj_t *txt = label(btn, "СТАРТ", ST_TEXT);
    lv_obj_set_style_text_font(txt, lv_font_36, 0);

    return btn;
}

static void show_modal(lv_obj_t *modal, bool show)
{
    if (show)
        lv_obj_clear_flag(modal, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(modal, LV_OBJ_FLAG_HIDDEN);
}

static void save_setpoint_event(lv_event_t *e)
{
    LV_UNUSED(e);
    char buf[96];
    show_modal(setpoint_modal, false);
    snprintf(buf, sizeof(buf), "Новая уставка давления: %.1f бар", set_pressure);
    set_message(auto_msg_label, buf);
    //  update_pressure_ui();
}

void hmi_create(uint8_t *font_buf, long buffer_size)
{
    // 1. Инициализируем стили с защитой от повторного вызова

    init_fonts(font_buf, buffer_size);
    hmi_styles.init();
    // register_manual_page_api();

    // 2. Сетки ДОЛЖНЫ быть static, так как LVGL хранит только указатель на них
    static lv_coord_t root_col_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t root_row_dsc[] = {64, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

    static lv_coord_t top_col_dsc[] = {320, LV_GRID_FR(1), 320, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t top_row_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

    static lv_style_t screen_font_style;
    lv_style_init(&screen_font_style);

    // 2. Set your desired font (e.g., Montserrat 24)
    lv_style_set_text_font(&screen_font_style, lv_font_24);
    lv_obj_add_style(lv_screen_active(), &screen_font_style, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Создание корневого контейнера
    root = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(root);
    lv_obj_add_style(root, &screen_font_style, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(root, ST_ROOT, 0);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_grid_dsc_array(root, root_col_dsc, root_row_dsc);

    // Верхняя панель (Топбар)
    lv_obj_t *top = lv_obj_create(root);
    lv_obj_add_style(top, ST_TOPBAR, 0);
    lv_obj_set_grid_cell(top, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_grid_dsc_array(top, top_col_dsc, top_row_dsc);

    // Левая часть топбара (Время и Сеть)
    lv_obj_t *left = lv_obj_create(top);
    lv_obj_remove_style_all(left);
    lv_obj_set_grid_cell(left, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(left, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(left, 20, 0);

    clock_label = label(left, "12:30", ST_TEXT);
    network_label = label(left, LV_SYMBOL_WIFI " Сеть: ● Подключено", ST_TEXT);
    lv_obj_set_style_text_color(network_label, C_GREEN, 0);

    // Заголовок экрана (Центр)
    screen_title = label(top, "АВТО РЕЖИМ", ST_TEXT);
    lv_obj_set_style_text_font(screen_title, lv_font_32, 0);
    lv_obj_set_grid_cell(screen_title, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    // Кнопка настроек (Справа)
    lv_obj_t *settings = button(top, LV_SYMBOL_SETTINGS " Настройки", ST_BTN_DARK);
    lv_obj_set_size(settings, 220, 46);
    lv_obj_set_grid_cell(settings, LV_GRID_ALIGN_END, 2, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_add_event_cb(settings, hmi_events.settings_event, LV_EVENT_CLICKED, NULL);

    // Основной контейнер контента
    lv_obj_t *content = lv_obj_create(root);
    lv_obj_remove_style_all(content);
    lv_obj_set_grid_cell(content, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);

    // Инициализация в hmi_create
    hmi_auto_page.create(content);
    hmi_manual_page.create(content);
    hmi_setpoint_modal_api.create(root);
    hmi_confirm_modal_api.create(root);
    hmi_settings.create(content);

    // Управление отображением по умолчанию
    hmi_auto_page.show();

    //  // Обновление состояний данных
    update_pressure_ui();

    hmi_manual_page.hide();

    //  auto_running = true;

    //  // 4. Защита от дублирования таймеров
    //  static lv_timer_t *pressure_timer = NULL;
    //  static lv_timer_t *clock_timer = NULL;

    //  if (!pressure_timer) pressure_timer = lv_timer_create(pressure_timer_cb, 900, NULL);
    //  if (!clock_timer)    clock_timer = lv_timer_create(clock_timer_cb, 1000, NULL);
}
