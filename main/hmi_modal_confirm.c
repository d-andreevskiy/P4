#include "hmi.h"

// --- ГЛОБАЛЬНЫЙ УКАЗАТЕЛЬ ОКНА ---
lv_obj_t *manual_confirm_modal = NULL;

// --- ПРОТОТИПЫ ЛОКАЛЬНЫХ МЕТОДОВ КЛАССА ---
static void modal_create(lv_obj_t *parent);
static void modal_show(void);
static void modal_hide(void);

// --- ГЛАВНЫЙ КОНСТРУКТОР ВЕРСТКИ ОКНА ПОДТВЕРЖДЕНИЯ ---
static void modal_create(lv_obj_t *parent)
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

    label(box, "ПЕРЕХОД В РУЧНОЙ РЕЖИМ", ST_TEXT);
    lv_obj_t *msg = label(box, "Продолжить?", ST_MUTED);
    lv_label_set_long_mode(msg, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(msg, LV_PCT(90));
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *actions = lv_obj_create(box);
    lv_obj_remove_style_all(actions);
    lv_obj_set_size(actions, LV_PCT(100), 64);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(actions, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *yes = button(actions, "ДА", ST_BTN_GREEN);
    lv_obj_set_size(yes, 180, 62);
    lv_obj_add_event_cb(yes, hmi_events.confirm_manual_event, LV_EVENT_CLICKED, NULL);

    lv_obj_t *no = button(actions, "НЕТ", ST_BTN_RED);
    lv_obj_set_size(no, 180, 62);
    lv_obj_add_event_cb(no, hmi_events.cancel_manual_event, LV_EVENT_CLICKED, NULL);
}

// --- МЕТОДЫ УПРАВЛЕНИЯ ВИДИМОСТЬЮ ---
static void modal_show(void)
{
    if (manual_confirm_modal)
    {
        lv_obj_clear_flag(manual_confirm_modal, LV_OBJ_FLAG_HIDDEN);
    }
}

static void modal_hide(void)
{
    if (manual_confirm_modal)
    {
        lv_obj_add_flag(manual_confirm_modal, LV_OBJ_FLAG_HIDDEN);
    }
}

// --- АВТОРЕГИСТРАЦИЯ API ОКНА ПОДТВЕРЖДЕНИЯ ---
struct hmi_confirm_modal_api_t hmi_confirm_modal_api;

__attribute__((constructor)) static void register_confirm_modal_api(void) {
    hmi_confirm_modal_api.create = modal_create;
    hmi_confirm_modal_api.show   = modal_show;
    hmi_confirm_modal_api.hide   = modal_hide;
}
