#include "hmi.h"

// --- ГЛОБАЛЬНЫЕ УКАЗАТЕЛИ ЭЛЕМЕНТОВ (доступны для таймеров и событий) ---
bool water_present = true;
lv_obj_t *water_icon = NULL;
lv_obj_t *water_label = NULL;
lv_obj_t *auto_start_btn = NULL;
lv_obj_t *auto_msg_label = NULL;
lv_obj_t *set_pressure_label = NULL;
lv_obj_t *actual_pressure_label = NULL;
lv_obj_t *set_pressure_bar = NULL;
lv_obj_t *actual_pressure_bar = NULL;
lv_obj_t *auto_page = NULL;

// --- ПРОТОТИПЫ ЛОКАЛЬНЫХ МЕТОДОВ КЛАССА ---
static void auto_page_create(lv_obj_t *parent);
static void auto_page_update(void);
static void auto_page_show(void);
static void auto_page_hide(void);
static lv_obj_t *create_pressure_card(lv_obj_t *parent, const char *title, bool is_setpoint);

// --- ГЛАВНЫЙ КОНСТРУКТОР ПАНЕЛИ АВТОМАТИКИ ---
static void auto_page_create(lv_obj_t *parent)
{
    // Сетки хранятся статически в RAM
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

    // Карточка фактического давления
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
    label(water, "ВОДА В СИСТЕМЕ", ST_TEXT);

    water_icon = label(water, LV_SYMBOL_OK " ", ST_BIG_GREEN);
    lv_obj_set_style_text_font(water_icon, lv_font_48, 0);

    water_label = label(water, water_present ? "ЕСТЬ" : "НЕТ", ST_BIG_GREEN);
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
    lv_obj_t *to_manual = button(side, LV_SYMBOL_NEW_LINE " РУЧНОЙ РЕЖИМ", ST_BTN_DARK);
    lv_obj_set_grid_cell(to_manual, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_add_event_cb(to_manual, hmi_events.to_manual_event, LV_EVENT_CLICKED, NULL);

    // Информационная панель текущего режима
    lv_obj_t *mode = panel(side);
    lv_obj_set_grid_cell(mode, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_set_flex_flow(mode, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(mode, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *mode_name = label(mode, "АВТО", ST_BIG_GREEN);
    lv_obj_set_style_text_font(mode_name, lv_font_32, 0);
    label(mode, "Режим работы", ST_MUTED);

    // Большая кнопка СТАРТ / СТОП
    auto_start_btn = big_start_button(side);
    lv_obj_set_grid_cell(auto_start_btn, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
    lv_obj_add_event_cb(auto_start_btn, hmi_events.auto_start_event, LV_EVENT_CLICKED, NULL);

    // Нижняя панель системных сообщений
    lv_obj_t *msg = panel(auto_page);
    lv_obj_set_grid_cell(msg, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_set_flex_flow(msg, LV_FLEX_FLOW_COLUMN);
    label(msg, "СООБЩЕНИЯ", ST_TEXT);

    auto_msg_label = label(msg, "", ST_TEXT);
    set_message(auto_msg_label, "Система в норме");
}

// --- УПРАВЛЕНИЕ ВИДИМОСТЬЮ И ОБНОВЛЕНИЕМ ---
static void auto_page_show(void)
{
    if (auto_page)
    {
        lv_obj_clear_flag(auto_page, LV_OBJ_FLAG_HIDDEN);
        auto_screen_active = true;
    }
}

static void auto_page_hide(void)
{
    if (auto_page)
    {
        lv_obj_add_flag(auto_page, LV_OBJ_FLAG_HIDDEN);
    }
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

    hmi_setpoint_modal_api.set_value(set_pressure);
}

// --- ВСПОМОГАТЕЛЬНЫЙ КОНСТРУКТОР КАРТОЧЕК ---
static lv_obj_t *create_pressure_card(lv_obj_t *parent, const char *title, bool is_setpoint)
{
    lv_obj_t *p = panel(parent);
    lv_obj_set_flex_flow(p, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(p, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_text_font(p, lv_font_22, 0);
    lv_obj_set_style_pad_gap(p, 8, 0);

    if (is_setpoint)
    {
        lv_obj_add_flag(p, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(p, hmi_events.open_setpoint_event, LV_EVENT_CLICKED, NULL);
        lv_obj_add_style(p, ST_PANEL_CLICK, 0);
    }
    else
    {
        lv_obj_clear_flag(p, LV_OBJ_FLAG_CLICKABLE);
    }

    lv_obj_t *t = label(p, title, ST_TEXT);
    lv_obj_set_style_text_font(t, lv_font_20, 0);

    lv_obj_t *row = lv_obj_create(p);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_PCT(100), 40);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(row, 1, 0);

    lv_obj_t *num = label(row, is_setpoint ? "10.0" : "7.8", is_setpoint ? ST_BIG_BLUE : ST_BIG_GREEN);
    lv_obj_t *unit = label(row, "бар", ST_MUTED);
    LV_UNUSED(unit);

    if (is_setpoint)
    {
        set_pressure_label = num;
        label(p, "Нажмите для изменения", ST_MUTED);
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
    lv_obj_set_style_bg_color(bar, is_setpoint ? lv_color_hex(0x3498DB) : lv_color_hex(0x2ECC71), LV_PART_INDICATOR);

    lv_obj_set_style_outline_width(bar, 2, LV_PART_MAIN);
    lv_obj_set_style_outline_color(bar, lv_color_hex(0x4E5E70), LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 12, LV_PART_MAIN);
    lv_obj_set_style_outline_pad(bar, 1, LV_PART_MAIN);

    if (is_setpoint)
        set_pressure_bar = bar;
    else
        actual_pressure_bar = bar;

    lv_obj_t *scale = lv_obj_create(p);
    lv_obj_set_style_bg_opa(scale, LV_OPA_TRANSP, 0);     
    lv_obj_set_style_border_opa(scale, LV_OPA_TRANSP, 0); 
    lv_obj_set_style_pad_all(scale, 0, 0);                
    lv_obj_set_size(scale, LV_PCT(92), 24);

    lv_obj_set_flex_flow(scale, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(scale, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *s0 = label(scale, "0", ST_TEXT);
    lv_obj_t *s16 = label(scale, "16", ST_TEXT);
    LV_UNUSED(s0);
    LV_UNUSED(s16);

    return p;
}

// --- АВТОРЕГИСТРАЦИЯ API АВТОМАТИЧЕСКОГО РЕЖИМА ---
struct hmi_auto_page_api hmi_auto_page;

__attribute__((constructor)) static void register_auto_page_api(void) {
    hmi_auto_page.create = auto_page_create;
    hmi_auto_page.update = update_pressure_ui;
    hmi_auto_page.show   = auto_page_show;
    hmi_auto_page.hide   = auto_page_hide;
}
