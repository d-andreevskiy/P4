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
static float set_pressure_old = 10.0f;

bool auto_running = false;
static bool manual_running = false;
static bool water_present = true;
bool auto_screen_active = true;

/* Объекты */
static lv_obj_t *root;
static lv_obj_t *screen_title;
lv_obj_t *clock_label;
static lv_obj_t *network_label;
static lv_obj_t *auto_page;
static lv_obj_t *manual_page;

static lv_obj_t *actual_pressure_label;
static lv_obj_t *actual_pressure_bar;
static lv_obj_t *set_pressure_label;
static lv_obj_t *set_pressure_bar;
static lv_obj_t *modal_pressure_label;
static lv_obj_t *water_label;
static lv_obj_t *water_icon;
lv_obj_t *auto_msg_label;
lv_obj_t *manual_msg_label;
lv_obj_t *auto_start_btn;
lv_obj_t *manual_start_btn;
static lv_obj_t *setpoint_modal;
static lv_obj_t *manual_confirm_modal;

static lv_obj_t *pump_slider[3];
static lv_obj_t *pump_value_label[3];

static lv_style_t st_root;
static lv_style_t st_topbar;
static lv_style_t st_panel;
static lv_style_t st_panel_clickable;
static lv_style_t st_btn_dark;
static lv_style_t st_btn_green;
static lv_style_t st_btn_red;
static lv_style_t st_btn_blue;
static lv_style_t st_btn_gray;
static lv_style_t st_text;
static lv_style_t st_muted;
static lv_style_t st_big_green;
static lv_style_t st_big_blue;
static lv_style_t st_big_orange;

void set_message(lv_obj_t *label, const char *text)
{
    char buf[160];
    snprintf(buf, sizeof(buf), LV_SYMBOL_OK "  %s", text);
    lv_label_set_text(label, buf);
}

static int pressure_to_percent(float value)
{
    if (value < 0)
        value = 0;
    if (value > HMI_PRESSURE_MAX)
        value = HMI_PRESSURE_MAX;
    return (int)((value / HMI_PRESSURE_MAX) * 100.0f + 0.5f);
}

void update_pressure_ui(void)
{
    char buf[24];

    snprintf(buf, sizeof(buf), "%.1f", actual_pressure);
    lv_label_set_text(actual_pressure_label, buf);
    lv_bar_set_value(actual_pressure_bar, pressure_to_percent(actual_pressure), LV_ANIM_ON);

    snprintf(buf, sizeof(buf), "%.1f", set_pressure);
    lv_label_set_text(set_pressure_label, buf);
    lv_bar_set_value(set_pressure_bar, pressure_to_percent(set_pressure), LV_ANIM_ON);

    if (modal_pressure_label)
    {
        snprintf(buf, sizeof(buf), "%.1f бар", set_pressure);
        lv_label_set_text(modal_pressure_label, buf);
    }
}

void update_big_button(lv_obj_t *btn, bool running)
{
    lv_obj_t *icon = lv_obj_get_child(btn, 0);
    lv_obj_t *text = lv_obj_get_child(btn, 1);

    lv_label_set_text(icon, running ? LV_SYMBOL_STOP : LV_SYMBOL_PLAY);
    lv_label_set_text(text, running ? "СТОП" : "СТАРТ");

    lv_obj_remove_style(btn, &st_btn_green, 0);
    lv_obj_remove_style(btn, &st_btn_red, 0);
    lv_obj_add_style(btn, running ? &st_btn_red : &st_btn_green, 0);
}

lv_obj_t *label(lv_obj_t *parent, const char *text, const lv_style_t *style)
{
    lv_obj_t *obj = lv_label_create(parent);
    lv_label_set_text(obj, text);
    if (style)
        lv_obj_add_style(obj, style, 0);
    return obj;
}

static lv_obj_t *panel(lv_obj_t *parent)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_add_style(obj, &st_panel, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

lv_obj_t *button(lv_obj_t *parent, const char *text, const lv_style_t *style)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_add_style(btn, style, 0);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *txt = label(btn, text, &st_text);
    lv_obj_center(txt);
    return btn;
}

static lv_obj_t *big_start_button(lv_obj_t *parent)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_add_style(btn, &st_btn_green, 0);
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(btn, 16, 0);

    lv_obj_t *icon = label(btn, LV_SYMBOL_PLAY, &st_text);
    lv_obj_set_style_text_font(icon, lv_font_36, 0);

    lv_obj_t *txt = label(btn, "СТАРТ", &st_text);
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

static void auto_start_event(lv_event_t *e)
{
    LV_UNUSED(e);
    auto_running = !auto_running;
    update_big_button(auto_start_btn, auto_running);
    set_message(auto_msg_label, auto_running ? "Автоматический режим запущен" : "Автоматический режим остановлен");
}

static void manual_start_event(lv_event_t *e)
{
    LV_UNUSED(e);
    manual_running = !manual_running;
    update_big_button(manual_start_btn, manual_running);
    set_message(manual_msg_label, manual_running ? "Ручной запуск активен" : "Ручной запуск остановлен");
}

static void open_manual_now(void)
{
    auto_running = false;
    update_big_button(auto_start_btn, false);
    auto_screen_active = false;
    lv_label_set_text(screen_title, "РУЧНОЙ РЕЖИМ");
    lv_obj_add_flag(auto_page, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(manual_page, LV_OBJ_FLAG_HIDDEN);
    set_message(manual_msg_label, "Ручной режим активирован");
}

static void open_auto_event(lv_event_t *e)
{
    LV_UNUSED(e);
    manual_running = false;
    update_big_button(manual_start_btn, false);
    auto_screen_active = true;
    lv_label_set_text(screen_title, "АВТО РЕЖИМ");
    lv_obj_add_flag(manual_page, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(auto_page, LV_OBJ_FLAG_HIDDEN);
    set_message(auto_msg_label, "Авто режим активирован");
}

static void to_manual_event(lv_event_t *e)
{
    LV_UNUSED(e);
    ESP_LOGI(TAG, "to_manual_event");
    //  if(auto_running)
    show_modal(manual_confirm_modal, true);
    //  else open_manual_now();
}

static void confirm_manual_event(lv_event_t *e)
{
    LV_UNUSED(e);
    if (manual_confirm_modal)
        show_modal(manual_confirm_modal, false);
    open_manual_now();
}

static void cancel_manual_event(lv_event_t *e)
{
    LV_UNUSED(e);
    if (manual_confirm_modal)
        show_modal(manual_confirm_modal, false);
}

static void open_setpoint_event(lv_event_t *e)
{
    LV_UNUSED(e);
    if (setpoint_modal)
        show_modal(setpoint_modal, true);
    ESP_LOGI(TAG, "%d: clicked", __LINE__);
    set_pressure_old = set_pressure;
}

static void pressure_minus_event(lv_event_t *e)
{
    LV_UNUSED(e);
    set_pressure -= HMI_SETPOINT_STEP;
    if (set_pressure < 0.0f)
        set_pressure = 0.0f;
    update_pressure_ui();
}

static void pressure_plus_event(lv_event_t *e)
{
    LV_UNUSED(e);
    set_pressure += HMI_SETPOINT_STEP;
    if (set_pressure > HMI_PRESSURE_MAX)
        set_pressure = HMI_PRESSURE_MAX;
    update_pressure_ui();
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

static void close_setpoint_event(lv_event_t *e)
{
    LV_UNUSED(e);
    show_modal(setpoint_modal, false);
    set_pressure = set_pressure_old;
    update_pressure_ui();
}

static void pump_slider_event(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    char buf[12];
    snprintf(buf, sizeof(buf), "%d%%", (int)lv_slider_get_value(slider));
    lv_label_set_text(pump_value_label[idx], buf);
}

static void pump_step_event(lv_event_t *e)
{
    int data = (int)(intptr_t)lv_event_get_user_data(e);
    int idx = data / 10;
    int step = data % 10;
    if (step == 9)
        step = -5;
    else
        step = 5;

    int value = lv_slider_get_value(pump_slider[idx]) + step;
    if (value < 0)
        value = 0;
    if (value > 100)
        value = 100;
    lv_slider_set_value(pump_slider[idx], value, LV_ANIM_ON);

    char buf[12];
    snprintf(buf, sizeof(buf), "%d%%", value);
    lv_label_set_text(pump_value_label[idx], buf);
}

void init_styles(void)
{
    lv_style_init(&st_root);
    lv_style_set_bg_color(&st_root, C_BG);
    lv_style_set_bg_opa(&st_root, LV_OPA_COVER);
    lv_style_set_text_color(&st_root, C_TEXT);

    lv_style_init(&st_topbar);
    lv_style_set_bg_color(&st_topbar, C_TOP);
    lv_style_set_bg_opa(&st_topbar, LV_OPA_COVER);
    lv_style_set_border_width(&st_topbar, 2);
    lv_style_set_border_color(&st_topbar, lv_color_hex(0x1F2B36));
    lv_style_set_pad_all(&st_topbar, 16);

    lv_style_init(&st_panel);
    lv_style_set_bg_color(&st_panel, C_PANEL);
    lv_style_set_bg_opa(&st_panel, LV_OPA_COVER);
    lv_style_set_border_width(&st_panel, 1);
    lv_style_set_border_color(&st_panel, C_BORDER);
    lv_style_set_radius(&st_panel, 10);
    lv_style_set_pad_all(&st_panel, 18);
    lv_style_set_shadow_width(&st_panel, 18);
    lv_style_set_shadow_opa(&st_panel, LV_OPA_40);

    lv_style_init(&st_panel_clickable);
    lv_style_set_border_color(&st_panel_clickable, C_BLUE);

    lv_style_init(&st_text);
    lv_style_set_text_color(&st_text, C_TEXT);
    lv_style_set_text_font(&st_text, lv_font_20);

    lv_style_init(&st_muted);
    lv_style_set_text_color(&st_muted, C_MUTED);
    lv_style_set_text_font(&st_muted, lv_font_22);

    lv_style_init(&st_big_green);
    lv_style_set_text_color(&st_big_green, C_GREEN);
    lv_style_set_text_font(&st_big_green, lv_font_48);

    lv_style_init(&st_big_blue);
    lv_style_set_text_color(&st_big_blue, C_BLUE);
    lv_style_set_text_font(&st_big_blue, lv_font_48);

    lv_style_init(&st_big_orange);
    lv_style_set_text_color(&st_big_orange, C_ORANGE);
    lv_style_set_text_font(&st_big_orange, lv_font_48);

    lv_style_init(&st_btn_dark);
    lv_style_set_bg_color(&st_btn_dark, C_PANEL_2);
    lv_style_set_bg_opa(&st_btn_dark, LV_OPA_COVER);
    lv_style_set_border_width(&st_btn_dark, 1);
    lv_style_set_border_color(&st_btn_dark, C_BORDER_LT);
    lv_style_set_radius(&st_btn_dark, 8);
    lv_style_set_pad_all(&st_btn_dark, 12);

    lv_style_init(&st_btn_green);
    lv_style_set_bg_color(&st_btn_green, lv_color_hex(0x1F9A3A));
    lv_style_set_bg_grad_color(&st_btn_green, lv_color_hex(0x39C95A));
    lv_style_set_bg_grad_dir(&st_btn_green, LV_GRAD_DIR_VER);
    lv_style_set_radius(&st_btn_green, 10);

    lv_style_init(&st_btn_red);
    lv_style_set_bg_color(&st_btn_red, lv_color_hex(0xB91515));
    lv_style_set_bg_grad_color(&st_btn_red, lv_color_hex(0xF33434));
    lv_style_set_bg_grad_dir(&st_btn_red, LV_GRAD_DIR_VER);
    lv_style_set_radius(&st_btn_red, 10);

    lv_style_init(&st_btn_blue);
    lv_style_set_bg_color(&st_btn_blue, lv_color_hex(0x1662C8));
    lv_style_set_bg_grad_color(&st_btn_blue, lv_color_hex(0x339CFF));
    lv_style_set_bg_grad_dir(&st_btn_blue, LV_GRAD_DIR_VER);
    lv_style_set_radius(&st_btn_blue, 8);

    lv_style_init(&st_btn_gray);
    lv_style_set_bg_color(&st_btn_gray, lv_color_hex(0x1C2732));
    lv_style_set_bg_grad_color(&st_btn_gray, lv_color_hex(0x334250));
    lv_style_set_bg_grad_dir(&st_btn_gray, LV_GRAD_DIR_VER);
}

// 1. Меняем void на lv_obj_t*
static lv_obj_t *create_pressure_card(lv_obj_t *parent, const char *title, bool is_setpoint)
{
    lv_obj_t *p = panel(parent);
    lv_obj_set_flex_flow(p, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(p, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_text_font(p, lv_font_22, 0);

    lv_obj_set_style_pad_gap(p, 8, 0);

    // 2. Явно настраиваем кликабельность карточки
    if (is_setpoint)
    {
        lv_obj_add_flag(p, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(p, open_setpoint_event, LV_EVENT_CLICKED, NULL);
        // Добавляем стиль кликабельной панели, который вы инициализировали в init_styles
        lv_obj_add_style(p, &st_panel_clickable, 0);
    }
    else
    {
        lv_obj_clear_flag(p, LV_OBJ_FLAG_CLICKABLE);
    }

    lv_obj_t *t = label(p, title, &st_text);
    lv_obj_set_style_text_font(t, lv_font_20, 0);

    lv_obj_t *row = lv_obj_create(p);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_PCT(100), 40);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(row, 1, 0);

    lv_obj_t *num = label(row, is_setpoint ? "10.0" : "7.8", is_setpoint ? &st_big_blue : &st_big_green);
    lv_obj_t *unit = label(row, "бар", &st_muted);
    LV_UNUSED(unit);

    if (is_setpoint)
    {
        set_pressure_label = num;
        label(p, "Нажмите для изменения", &st_muted);
    }
    else
    {
        actual_pressure_label = num;
    }

    lv_obj_t *bar = lv_bar_create(p);
    lv_obj_set_style_pad_top(bar, 0, 0);
    lv_obj_set_style_pad_bottom(bar, 0, 0);
    lv_obj_set_size(bar, LV_PCT(92), 20);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 50, LV_ANIM_ON);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x26313D), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, is_setpoint ? C_BLUE : C_GREEN, LV_PART_INDICATOR);

    lv_obj_set_style_outline_width(bar, 2, LV_PART_MAIN);
    lv_obj_set_style_outline_color(bar, lv_color_hex(0x4E5E70), LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 12, LV_PART_MAIN);
    lv_obj_set_style_outline_pad(bar, 1, LV_PART_MAIN);

    if (is_setpoint)
        set_pressure_bar = bar;
    else
        actual_pressure_bar = bar;
    lv_obj_t *scale = lv_obj_create(p);

    // 2. Делаем объект полностью прозрачным, обнуляя фон, рамки и отступы
    lv_obj_set_style_bg_opa(scale, LV_OPA_TRANSP, 0);     // Прячем серый фон
    lv_obj_set_style_border_opa(scale, LV_OPA_TRANSP, 0); // Прячем рамку
    lv_obj_set_style_pad_all(scale, 0, 0);                // Убираем скрытые отступы

    // 3. Жестко фиксируем размеры (Ширина 92%, высота 24 пикселя)
    lv_obj_set_size(scale, LV_PCT(92), 24);

    // 4. Включаем Flex, чтобы разнести ноль и 16 по краям полосы
    lv_obj_set_flex_flow(scale, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(scale, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // 5. Создаем сами текстовые метки (больше никакого lv_obj_align не требуется)
    lv_obj_t *s0 = label(scale, "0", &st_text);
    lv_obj_t *s16 = label(scale, "16", &st_text);
    LV_UNUSED(s0);
    LV_UNUSED(s16);

    // 3. Возвращаем указатель на созданную карточку
    return p;
}

static void create_auto_page(lv_obj_t *parent)
{
    // Функция теперь возвращает lv_obj_t* для менеджера экранов

    // 1. Переводим ВСЕ сетки в static, чтобы они постоянно хранились в RAM
    static lv_coord_t auto_page_cols[] = {LV_GRID_FR(1), 300, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t auto_page_rows[] = {LV_GRID_FR(1), 150, LV_GRID_TEMPLATE_LAST};

    static lv_coord_t main_cols[] = {LV_GRID_FR(11), LV_GRID_FR(7), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t main_rows[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

    static lv_coord_t side_cols[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t side_rows[] = {98, 150, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

    // Создание корневого контейнера страницы
    auto_page = lv_obj_create(parent);
    lv_obj_remove_style_all(auto_page);
    lv_obj_set_size(auto_page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_grid_dsc_array(auto_page, auto_page_cols, auto_page_rows);
    lv_obj_set_style_pad_all(auto_page, 12, 0);
    lv_obj_set_style_pad_gap(auto_page, 10, 0);

    // Левая область контента (Главная)
    lv_obj_t *main = lv_obj_create(auto_page);
    lv_obj_remove_style_all(main);
    lv_obj_set_grid_cell(main, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_set_grid_dsc_array(main, main_cols, main_rows);
    lv_obj_set_style_pad_gap(main, 16, 0);

    // 2. БЕЗОПАСНО: сохраняем указатели из функций вместо lv_obj_get_child
    lv_obj_t *card_actual_pres = create_pressure_card(main, "ФАКТИЧЕСКОЕ ДАВЛЕНИЕ", false);
    if (card_actual_pres)
    {
        lv_obj_set_grid_cell(card_actual_pres, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    }

    // Карточка состояния воды
    lv_obj_t *water = panel(main);
    lv_obj_set_grid_cell(water, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_set_flex_flow(water, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(water, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    label(water, "ВОДА В СИСТЕМЕ", &st_text);

    water_icon = label(water, LV_SYMBOL_OK " ", &st_big_green);
    lv_obj_set_style_text_font(water_icon, lv_font_48, 0);

    water_label = label(water, water_present ? "ЕСТЬ" : "НЕТ", &st_big_green);
    lv_obj_set_style_text_font(water_label, lv_font_24, 0);

    // Карточка заданного давления
    lv_obj_t *card_target_pres = create_pressure_card(main, "УСТАНОВЛЕННОЕ ДАВЛЕНИЕ", true);
    if (card_target_pres)
    {
        lv_obj_set_grid_cell(card_target_pres, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    }

    // Правая область (Боковая панель управления)
    lv_obj_t *side = lv_obj_create(auto_page);
    lv_obj_remove_style_all(side);
    lv_obj_set_grid_cell(side, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 2);
    lv_obj_set_grid_dsc_array(side, side_cols, side_rows);
    lv_obj_set_style_pad_gap(side, 20, 0);

    // Кнопка перехода в ручной режим
    lv_obj_t *to_manual = button(side, LV_SYMBOL_NEW_LINE " РУЧНОЙ РЕЖИМ", &st_btn_dark);
    lv_obj_set_grid_cell(to_manual, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_add_event_cb(to_manual, to_manual_event, LV_EVENT_CLICKED, NULL);

    // Информационная панель текущего режима
    lv_obj_t *mode = panel(side);
    lv_obj_set_grid_cell(mode, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_set_flex_flow(mode, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(mode, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *mode_name = label(mode, "АВТО", &st_big_green);
    lv_obj_set_style_text_font(mode_name, lv_font_32, 0);
    label(mode, "Режим работы", &st_muted);

    // Большая кнопка СТАРТ / СТОП
    auto_start_btn = big_start_button(side);
    lv_obj_set_grid_cell(auto_start_btn, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 2, 1);

    lv_obj_add_event_cb(auto_start_btn, auto_start_event, LV_EVENT_CLICKED, NULL);

    // Нижняя панель системных сообщений
    lv_obj_t *msg = panel(auto_page);
    lv_obj_set_grid_cell(msg, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_set_flex_flow(msg, LV_FLEX_FLOW_COLUMN);
    label(msg, "СООБЩЕНИЯ", &st_text);

    auto_msg_label = label(msg, "", &st_text);
    set_message(auto_msg_label, "Система в норме");

    // return auto_page;
}

static void create_pump_card(lv_obj_t *parent, int idx, const char *title, int init_value, lv_color_t color)
{
    lv_obj_t *p = panel(parent);
    lv_obj_set_flex_flow(p, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(p, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *head = label(p, title, &st_text);
    lv_obj_set_style_text_font(head, lv_font_22, 0);

    lv_obj_t *val = label(p, "", idx == 0 ? &st_big_green : (idx == 1 ? &st_big_blue : &st_big_orange));
    pump_value_label[idx] = val;

    char buf[12];
    snprintf(buf, sizeof(buf), "%d%%", init_value);
    lv_label_set_text(val, buf);

    label(p, "Скорость", &st_muted);

    pump_slider[idx] = lv_slider_create(p);
    lv_obj_set_size(pump_slider[idx], LV_PCT(92), 26);
    lv_slider_set_range(pump_slider[idx], 0, 100);
    lv_slider_set_value(pump_slider[idx], init_value, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(pump_slider[idx], color, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(pump_slider[idx], color, LV_PART_KNOB);
    lv_obj_add_event_cb(pump_slider[idx], pump_slider_event, LV_EVENT_VALUE_CHANGED, (void *)(intptr_t)idx);

    lv_obj_t *row = lv_obj_create(p);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_PCT(100), 62);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *minus = button(row, "−", &st_btn_dark);
    lv_obj_set_size(minus, 70, 60);
    lv_obj_add_event_cb(minus, pump_step_event, LV_EVENT_CLICKED, (void *)(intptr_t)(idx * 10 + 9));

    lv_obj_t *plus = button(row, "+", &st_btn_dark);
    lv_obj_set_size(plus, 70, 60);
    lv_obj_add_event_cb(plus, pump_step_event, LV_EVENT_CLICKED, (void *)(intptr_t)(idx * 10 + 5));
}

void create_manual_page(lv_obj_t *parent)
{
    manual_page = lv_obj_create(parent);
    lv_obj_remove_style_all(manual_page);
    lv_obj_set_size(manual_page, LV_PCT(100), LV_PCT(100));
    lv_obj_add_flag(manual_page, LV_OBJ_FLAG_HIDDEN);

    // Безопасные статические массивы int32_t для LVGL v9 вместо временных (lv_coord_t[])
    static const int32_t page_cols[] = {LV_GRID_FR(1), 300, LV_GRID_TEMPLATE_LAST};
    static const int32_t page_rows[] = {LV_GRID_FR(1), 170, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(manual_page, page_cols, page_rows);

    lv_obj_set_style_pad_all(manual_page, 22, 0);
    lv_obj_set_style_pad_gap(manual_page, 20, 0); // Вернули обратно

    // --- 2. Контейнер для КАРТОЧЕК НАСОСОВ ---
    lv_obj_t *main = lv_obj_create(manual_page);
    lv_obj_remove_style_all(main);
    lv_obj_set_grid_cell(main, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

    static const int32_t main_cols[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static const int32_t main_rows[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(main, main_cols, main_rows);

    lv_obj_set_style_pad_gap(main, 20, 0); // Вернули обратно

    create_pump_card(main, 0, "НАСОС 1", 60, C_GREEN);
    lv_obj_set_grid_cell(lv_obj_get_child(main, 0), LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    create_pump_card(main, 1, "НАСОС 2", 30, C_BLUE);
    lv_obj_set_grid_cell(lv_obj_get_child(main, 1), LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    create_pump_card(main, 2, "НАСОС 3", 80, C_ORANGE);
    lv_obj_set_grid_cell(lv_obj_get_child(main, 2), LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

    // --- 3. Правый боковой сайдбар (SIDEBAR) ---
    lv_obj_t *side = lv_obj_create(manual_page);
    lv_obj_remove_style_all(side);
    lv_obj_set_grid_cell(side, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 2);

    static const int32_t side_cols[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static const int32_t side_rows[] = {98, 150, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(side, side_cols, side_rows);

    lv_obj_set_style_pad_gap(side, 20, 0); // Вернули обратно

    lv_obj_t *to_auto = button(side, "АВТО РЕЖИМ", &st_btn_dark);
    lv_obj_set_grid_cell(to_auto, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_add_event_cb(to_auto, open_auto_event, LV_EVENT_CLICKED, NULL);

    lv_obj_t *mode = panel(side);
    lv_obj_set_grid_cell(mode, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_set_flex_flow(mode, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(mode, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *mode_name = label(mode, "РУЧНОЙ", &st_big_green);
    lv_obj_set_style_text_font(mode_name, lv_font_32, 0);
    label(mode, "Режим работы", &st_muted);

    manual_start_btn = big_start_button(side);
    lv_obj_set_grid_cell(manual_start_btn, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
    lv_obj_add_event_cb(manual_start_btn, manual_start_event, LV_EVENT_CLICKED, NULL);

    // --- 4. Нижний блок СООБЩЕНИЙ ---
    lv_obj_t *msg_panel = panel(manual_page);
    lv_obj_set_grid_cell(msg_panel, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_set_flex_flow(msg_panel, LV_FLEX_FLOW_COLUMN);

    label(msg_panel, "СООБЩЕНИЯ", &st_text);
    manual_msg_label = label(msg_panel, "", &st_text);

    // Безопасная прямая установка текста
    lv_label_set_text(manual_msg_label, "Ручной режим активирован");
}

void create_setpoint_modal(lv_obj_t *parent)
{
    setpoint_modal = lv_obj_create(parent);
    lv_obj_set_size(setpoint_modal, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(setpoint_modal, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(setpoint_modal, LV_OPA_70, 0);
    lv_obj_clear_flag(setpoint_modal, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(setpoint_modal, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *box = panel(setpoint_modal);
    lv_obj_set_size(box, 450, 330);
    lv_obj_center(box);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(box, 14, 0);

    label(box, "УСТАНОВКА ДАВЛЕНИЯ", &st_text);
    modal_pressure_label = label(box, "10.0 бар", &st_big_blue);
    label(box, "Диапазон: 0-16 бар", &st_muted);

    lv_obj_t *step = lv_obj_create(box);
    lv_obj_remove_style_all(step);
    lv_obj_set_size(step, LV_PCT(100), 64);
    lv_obj_set_flex_flow(step, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(step, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *minus = button(step, "-", &st_btn_gray);
    lv_obj_set_size(minus, 170, 62);
    lv_obj_add_event_cb(minus, pressure_minus_event, LV_EVENT_CLICKED, NULL);

    lv_obj_t *plus = button(step, "+", &st_btn_blue);
    lv_obj_set_size(plus, 170, 62);
    lv_obj_add_event_cb(plus, pressure_plus_event, LV_EVENT_CLICKED, NULL);

    lv_obj_t *actions = lv_obj_create(box);
    lv_obj_remove_style_all(actions);
    lv_obj_set_size(actions, LV_PCT(100), 64);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(actions, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *ok = button(actions, "ОК", &st_btn_green);
    lv_obj_set_size(ok, 170, 62);
    lv_obj_add_event_cb(ok, save_setpoint_event, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cancel = button(actions, "ОТМЕНА", &st_btn_red);
    lv_obj_set_size(cancel, 170, 62);
    lv_obj_add_event_cb(cancel, close_setpoint_event, LV_EVENT_CLICKED, NULL);
}

void create_manual_confirm_modal(lv_obj_t *parent)
{
    manual_confirm_modal = lv_obj_create(parent);
    lv_obj_set_size(manual_confirm_modal, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(manual_confirm_modal, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(manual_confirm_modal, LV_OPA_70, 0);
    lv_obj_clear_flag(manual_confirm_modal, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(manual_confirm_modal, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *box = panel(manual_confirm_modal);
    lv_obj_set_size(box, 480, 260);
    lv_obj_center(box);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(box, 18, 0);

    label(box, "ПЕРЕХОД В РУЧНОЙ РЕЖИМ", &st_text);
    lv_obj_t *msg = label(box, "Продолжить?", &st_muted);
    lv_label_set_long_mode(msg, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(msg, LV_PCT(90));
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *actions = lv_obj_create(box);
    lv_obj_remove_style_all(actions);
    lv_obj_set_size(actions, LV_PCT(100), 64);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(actions, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *yes = button(actions, "ДА", &st_btn_green);
    lv_obj_set_size(yes, 180, 62);
    lv_obj_add_event_cb(yes, confirm_manual_event, LV_EVENT_CLICKED, NULL);

    lv_obj_t *no = button(actions, "НЕТ", &st_btn_red);
    lv_obj_set_size(no, 180, 62);
    lv_obj_add_event_cb(no, cancel_manual_event, LV_EVENT_CLICKED, NULL);
}


void hmi_create(uint8_t *font_buf, long buffer_size)
{
    // 1. Инициализируем стили с защитой от повторного вызова

    init_fonts(font_buf, buffer_size);
    init_styles();

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
    lv_obj_add_style(root, &st_root, 0);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_grid_dsc_array(root, root_col_dsc, root_row_dsc);

    // Верхняя панель (Топбар)
    lv_obj_t *top = lv_obj_create(root);
    lv_obj_add_style(top, &st_topbar, 0);
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

    clock_label = label(left, "12:30", &st_text);
    network_label = label(left, LV_SYMBOL_WIFI " Сеть: ● Подключено", &st_text);
    lv_obj_set_style_text_color(network_label, C_GREEN, 0);

    // Заголовок экрана (Центр)
    screen_title = label(top, "АВТО РЕЖИМ", &st_text);
    lv_obj_set_style_text_font(screen_title, lv_font_32, 0);
    lv_obj_set_grid_cell(screen_title, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    // Кнопка настроек (Справа)
    lv_obj_t *settings = button(top, LV_SYMBOL_SETTINGS " Настройки", &st_btn_dark);
    lv_obj_set_size(settings, 220, 46);
    lv_obj_set_grid_cell(settings, LV_GRID_ALIGN_END, 2, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_add_event_cb(settings, hmi_events.settings_event, LV_EVENT_CLICKED, NULL);

    // Основной контейнер контента
    lv_obj_t *content = lv_obj_create(root);
    lv_obj_remove_style_all(content);
    lv_obj_set_grid_cell(content, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);

    // Инициализация страниц (функции должны возвращать lv_obj_t*)
    create_auto_page(content);

    lv_obj_clear_flag(auto_page, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(screen_title, "АВТО РЕЖИМ");

    create_manual_page(content);
    lv_obj_add_flag(manual_page, LV_OBJ_FLAG_HIDDEN);

    // 3. Устраняем наложение: по умолчанию показываем Авто, скрываем Ручной
    //  if (auto_page && manual_page) {
    //      lv_obj_clear_flag(auto_page, LV_OBJ_FLAG_HIDDEN);
    //      lv_obj_add_flag(manual_page, LV_OBJ_FLAG_HIDDEN);
    //  }

    //  // Модальные окна создаются поверх контента
    create_setpoint_modal(root);
    create_manual_confirm_modal(root);

    //  // Обновление состояний данных
    update_pressure_ui();
    update_big_button(auto_start_btn, false);
    update_big_button(manual_start_btn, false);

    //  auto_running = true;

    //  // 4. Защита от дублирования таймеров
    //  static lv_timer_t *pressure_timer = NULL;
    //  static lv_timer_t *clock_timer = NULL;

    //  if (!pressure_timer) pressure_timer = lv_timer_create(pressure_timer_cb, 900, NULL);
    //  if (!clock_timer)    clock_timer = lv_timer_create(clock_timer_cb, 1000, NULL);
}
