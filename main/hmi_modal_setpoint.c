
#include "hmi.h"

// Локальный указатель на контейнер модального окна
static lv_obj_t *setpoint_modal_obj = NULL;

// Прототипы локальных методов класса
static void modal_create(lv_obj_t *parent);
static void modal_show(void);
static void modal_hide(void);

static lv_obj_t *modal_pressure_label;


void create_setpoint_modal(lv_obj_t *parent)
{
    setpoint_modal_obj = lv_obj_create(parent);
    lv_obj_set_size(setpoint_modal_obj, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(setpoint_modal_obj, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(setpoint_modal_obj, LV_OPA_70, 0);
    lv_obj_clear_flag(setpoint_modal_obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(setpoint_modal_obj, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *box = panel(setpoint_modal_obj);
    lv_obj_set_size(box, 450, 330);
    lv_obj_center(box);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(box, 14, 0);

    label(box, "УСТАНОВКА ДАВЛЕНИЯ", ST_TEXT);
    modal_pressure_label = label(box, "10.0 бар", ST_BIG_BLUE);
    label(box, "Диапазон: 0-16 бар", ST_MUTED);

    lv_obj_t *step = lv_obj_create(box);
    lv_obj_remove_style_all(step);
    lv_obj_set_size(step, LV_PCT(100), 64);
    lv_obj_set_flex_flow(step, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(step, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *minus = button(step, "-", ST_BTN_GRAY);
    lv_obj_set_size(minus, 170, 62);
    lv_obj_add_event_cb(minus, hmi_events.pressure_minus_event, LV_EVENT_CLICKED, NULL);

    lv_obj_t *plus = button(step, "+", ST_BTN_BLUE);
    lv_obj_set_size(plus, 170, 62);
    lv_obj_add_event_cb(plus, hmi_events.pressure_plus_event, LV_EVENT_CLICKED, NULL);

    lv_obj_t *actions = lv_obj_create(box);
    lv_obj_remove_style_all(actions);
    lv_obj_set_size(actions, LV_PCT(100), 64);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(actions, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *ok = button(actions, "ОК", ST_BTN_GREEN);
    lv_obj_set_size(ok, 170, 62);
    lv_obj_add_event_cb(ok, hmi_events.save_setpoint_event, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cancel = button(actions, "ОТМЕНА", ST_BTN_RED);
    lv_obj_set_size(cancel, 170, 62);
    lv_obj_add_event_cb(cancel, hmi_events.close_setpoint_event, LV_EVENT_CLICKED, NULL);
}

// Методы управления видимостью
static void modal_show(void)
{
    if (setpoint_modal_obj)
    {
        lv_obj_clear_flag(setpoint_modal_obj, LV_OBJ_FLAG_HIDDEN);
    }
}

static void modal_hide(void)
{
    if (setpoint_modal_obj)
    {
        lv_obj_add_flag(setpoint_modal_obj, LV_OBJ_FLAG_HIDDEN);
    }
}

static void modal_set_value(float value)
{
    if (modal_pressure_label)
    {
        char buf[24];
        snprintf(buf, sizeof(buf), "%.1f бар", value);
        lv_label_set_text(modal_pressure_label, buf);
    }
}

// --- АВТОРЕГИСТРАЦИЯ API МОДАЛЬНОГО ОКНА ---
struct hmi_setpoint_modal_api hmi_setpoint_modal_api;

__attribute__((constructor)) static void register_setpoint_modal_api(void) {
    hmi_setpoint_modal_api.create = create_setpoint_modal;
    hmi_setpoint_modal_api.show   = modal_show;
    hmi_setpoint_modal_api.hide   = modal_hide;
    hmi_setpoint_modal_api.set_value = modal_set_value;


}