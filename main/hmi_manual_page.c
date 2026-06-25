#include "lvgl.h"
#include <stdio.h>
#include <stdbool.h>
#include "hmi.h"
#include "esp_log.h"


#define TAG "Manual page"
// --- Приватный контекст страницы (статические виджеты) ---
static lv_obj_t *manual_page, *manual_start_btn;
static lv_obj_t *pump_slider[3], *pump_value_label[3];
static lv_obj_t *manual_msg_label; // <-- Наш целевой указатель на manual_msg_label в структуре

// lv_obj_t *manual_start_btn;

static void manual_page_set_message(const char *text) {
    if (manual_msg_label != NULL) {
        set_message(manual_msg_label, text);
    }
}

static void manual_start_event(lv_event_t *e)
{
    LV_UNUSED(e);
    manual_running = !manual_running;
    update_big_button(manual_start_btn, manual_running);
    manual_page_set_message( manual_running ? "Ручной запуск активен" : "Ручной запуск остановлен");
}

// --- Внутренние коллбеки (логика изменения скорости) ---
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

// --- Конструктор карточки насоса ---
static void create_pump_card(lv_obj_t *parent, int idx, const char *title, int init_value, lv_color_t color)
{
    lv_obj_t *p = panel(parent);
    lv_obj_set_flex_flow(p, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(p, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *head = label(p, title, ST_TEXT);
    lv_obj_set_style_text_font(head, lv_font_22, 0);

    lv_obj_t *val = label(p, "", idx == 0 ? ST_BIG_GREEN : (idx == 1 ? ST_BIG_BLUE : ST_BIG_ORANGE));
    pump_value_label[idx] = val;

    char buf[12];
    snprintf(buf, sizeof(buf), "%d%%", init_value);
    lv_label_set_text(val, buf);

    label(p, "Скорость", ST_MUTED);

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

    lv_obj_t *minus = button(row, "-", ST_BTN_DARK);
    lv_obj_set_size(minus, 70, 60);
    lv_obj_add_event_cb(minus, pump_step_event, LV_EVENT_CLICKED, (void *)(intptr_t)(idx * 10 + 9));

    lv_obj_t *plus = button(row, "+", ST_BTN_DARK);
    lv_obj_set_size(plus, 70, 60);
    lv_obj_add_event_cb(plus, pump_step_event, LV_EVENT_CLICKED, (void *)(intptr_t)(idx * 10 + 5));
}

// --- Публичный API модуля ---
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

    lv_obj_t *to_auto = button(side, "АВТО РЕЖИМ", ST_BTN_DARK);
    lv_obj_set_grid_cell(to_auto, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_add_event_cb(to_auto, hmi_events.open_auto_event, LV_EVENT_CLICKED, NULL);

    lv_obj_t *mode = panel(side);
    lv_obj_set_grid_cell(mode, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_set_flex_flow(mode, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(mode, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *mode_name = label(mode, "РУЧНОЙ", ST_BIG_GREEN);
    lv_obj_set_style_text_font(mode_name, lv_font_32, 0);
    label(mode, "Режим работы", ST_MUTED);

    manual_start_btn = big_start_button(side);
    lv_obj_set_grid_cell(manual_start_btn, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
    lv_obj_add_event_cb(manual_start_btn, manual_start_event, LV_EVENT_CLICKED, NULL);

    // --- 4. Нижний блок СООБЩЕНИЙ ---
    lv_obj_t *msg_panel = panel(manual_page);
    lv_obj_set_grid_cell(msg_panel, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_set_flex_flow(msg_panel, LV_FLEX_FLOW_COLUMN);

    label(msg_panel, "СООБЩЕНИЯ", ST_TEXT);
    manual_msg_label = label(msg_panel, "", ST_TEXT);

    // Безопасная прямая установка текста
    hmi_manual_page.set_message( "Ручной режим активирован");
}


static void manual_page_update_status(bool running) { /* Обновление кнопки старта */ }
static void manual_page_update_pump(int idx, int value) { /* Обновление слайдера извне */ }

static void manual_page_show(void) {
    ESP_LOGI(TAG, "%s", __func__);
    if (manual_page)
        lv_obj_clear_flag(manual_page, LV_OBJ_FLAG_HIDDEN);
    else 
        ESP_LOGI(TAG, "%s manual_page is NULL ", __func__);

}

static void manual_page_hide(void) {
    ESP_LOGI(TAG, "%s", __func__);

    if (manual_page)
        lv_obj_add_flag(manual_page, LV_OBJ_FLAG_HIDDEN);
}

// --- Авторегистрация API ---
struct hmi_manual_page_api hmi_manual_page;

__attribute__((constructor)) static void register_manual_page_api(void) {
    hmi_manual_page.create = create_manual_page;
    hmi_manual_page.update_status = manual_page_update_status;
    hmi_manual_page.update_pump = manual_page_update_pump;
    hmi_manual_page.show = manual_page_show; // Регистрируем
    hmi_manual_page.hide = manual_page_hide; // Регистрируем
    hmi_manual_page.set_message = manual_page_set_message;
}