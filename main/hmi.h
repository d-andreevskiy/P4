#ifndef HMI_H
#define HMI_H

#include "lvgl.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define HMI_PRESSURE_MAX 10.0f

/* Цвета исходного интерфейса */
#define C_BG lv_color_hex(0x0B1118)
#define C_TOP lv_color_hex(0x080D13)
#define C_PANEL lv_color_hex(0x17212C)
#define C_PANEL_2 lv_color_hex(0x1D2935)
#define C_PANEL_3 lv_color_hex(0x111922)
#define C_BORDER lv_color_hex(0x334250)
#define C_BORDER_LT lv_color_hex(0x4B5B6B)
#define C_TEXT lv_color_hex(0xF3F7FB)
#define C_MUTED lv_color_hex(0xB8C1CC)
#define C_BLUE lv_color_hex(0x48A7FF)
#define C_ORANGE lv_color_hex(0xFF982D)
#define C_RED lv_color_hex(0xE22D2D)

#define C_GREEN          lv_color_hex(0x2ECC71)


// Переменные из hmi.c

extern bool manual_running;

extern lv_font_t *lv_font_20;
extern lv_font_t *lv_font_22;
extern lv_font_t *lv_font_24;
extern lv_font_t *lv_font_32;
extern lv_font_t *lv_font_36;
extern lv_font_t *lv_font_48;


extern bool auto_screen_active;
extern bool auto_running;
extern float actual_pressure;
extern float set_pressure;
extern lv_obj_t *clock_label;
extern lv_obj_t *auto_msg_label;
// extern lv_obj_t *manual_msg_label;
extern lv_obj_t *auto_start_btn;
// extern lv_obj_t *manual_start_btn;

extern lv_obj_t *screen_title;

typedef struct {
    lv_style_t st_root;
    lv_style_t st_topbar;
    lv_style_t st_panel;
    lv_style_t st_panel_clickable;
    lv_style_t st_btn_dark;
    lv_style_t st_btn_green;
    lv_style_t st_btn_red;
    lv_style_t st_btn_blue;
    lv_style_t st_btn_gray;
    lv_style_t st_text;
    lv_style_t st_muted;
    lv_style_t st_big_green;
    lv_style_t st_big_blue;
    lv_style_t st_big_orange;
    void (*init)(void);

#define ST_ROOT            (&hmi_styles.st_root)
#define ST_TOPBAR          (&hmi_styles.st_topbar)
#define ST_PANEL           (&hmi_styles.st_panel)
#define ST_PANEL_CLICK     (&hmi_styles.st_panel_clickable)
#define ST_BTN_DARK        (&hmi_styles.st_btn_dark)
#define ST_BTN_GREEN       (&hmi_styles.st_btn_green)
#define ST_BTN_RED         (&hmi_styles.st_btn_red)
#define ST_BTN_BLUE        (&hmi_styles.st_btn_blue)
#define ST_BTN_GRAY        (&hmi_styles.st_btn_gray)
#define ST_TEXT            (&hmi_styles.st_text)
#define ST_MUTED           (&hmi_styles.st_muted)
#define ST_BIG_GREEN       (&hmi_styles.st_big_green)
#define ST_BIG_BLUE        (&hmi_styles.st_big_blue)
#define ST_BIG_ORANGE      (&hmi_styles.st_big_orange)
 
} hmi_styles_t;

extern hmi_styles_t hmi_styles;

// Структура-"интерфейс" для вызова static функций
// --- API СИСТЕМНЫХ СОБЫТИЙ И ТАЙМЕРОВ ---
struct hmi_events_t {
    void (*settings_event)(lv_event_t *e);
    void (*pressure_timer_cb)(lv_timer_t *timer);
    void (*clock_timer_cb)(lv_timer_t *timer);
    void (*open_setpoint_event)(lv_event_t *e);
    void (*close_setpoint_event)(lv_event_t *e);
    void (*save_setpoint_event)(lv_event_t *e);
    void (*pressure_minus_event)(lv_event_t *e);
    void (*pressure_plus_event)(lv_event_t *e);
    void (*to_manual_event)(lv_event_t *e);
    void (*auto_start_event)(lv_event_t *e);
    void (*confirm_manual_event)(lv_event_t *e);
    void (*cancel_manual_event)(lv_event_t *e);
    void (*hmi_events)(lv_event_t *e);
    void (*open_auto_event)(lv_event_t *e);
};

// Экспортируем объект (без const, чтобы отработал конструктор авторегистрации)
extern struct hmi_events_t hmi_events;

struct hmi_setpoint_modal_api {
    void (*create)(lv_obj_t *parent);
    void (*show)(void);
    void (*hide)(void);
    void (*set_value)(float value); // Новая функция-обертка
};

extern struct hmi_setpoint_modal_api hmi_setpoint_modal_api;

// 1. Описываем API для окна подтверждения ручного режима
struct hmi_confirm_modal_api_t {
    void (*create)(lv_obj_t *parent);
    void (*show)(void);
    void (*hide)(void);
};
extern struct hmi_confirm_modal_api_t hmi_confirm_modal_api;

// Внешние функции отрисовки
extern void init_fonts(uint8_t *font_buf, long buffer_size);
// extern void init_styles(void);
// extern void create_auto_page(lv_obj_t *parent);

extern void create_manual_page(lv_obj_t *parent);
// extern void create_setpoint_modal(lv_obj_t *parent);
// extern void create_manual_confirm_modal(lv_obj_t *parent);
extern void update_pressure_ui(void);
extern void update_big_button(lv_obj_t *btn, bool state);
extern void set_message(lv_obj_t *label_obj, const char *text);
extern lv_obj_t * label(lv_obj_t * parent, const char * text, const lv_style_t * style);
extern lv_obj_t * button(lv_obj_t * parent, const char * text, const lv_style_t * style);
extern lv_obj_t *panel(lv_obj_t *parent);
// extern void open_auto_event(lv_event_t *e);

extern bool manual_running;

// extern lv_obj_t *manual_msg_label;

// --- API АВТОМАТИЧЕСКОГО РЕЖИМА ---
struct hmi_auto_page_api {
    void (*create)(lv_obj_t *parent);
    void (*update)(void);
    void (*show)(void);
    void (*hide)(void);
};
extern struct hmi_auto_page_api hmi_auto_page;

struct hmi_manual_page_api {
    void (*create)(lv_obj_t *parent);
    void (*update_status)(bool running);
    void (*update_pump)(int idx, int value);
    // Новые методы управления видимостью:
    void (*show)(void);
    void (*hide)(void);
    void (*set_message)( const char *text); 
};
extern struct hmi_manual_page_api hmi_manual_page;


// --- КЛАСС АВТОМАТИЧЕСКОГО РЕЖИМА ---
typedef struct {
    void (*create)(lv_obj_t *parent); // Верстка панели автоматики
    void (*update)(void);             // Обновление давления и датчиков на ходу
} hmi_auto_t;

extern const hmi_auto_t hmi_auto;


// extern void register_manual_page_api(void);

void hmi_create(uint8_t *font_buf, long buffer_size);
extern lv_obj_t *big_start_button(lv_obj_t *parent);

// Маркеры присутствия модулей для предотвращения оптимизации линкера
extern int hmi_auto_page_anchor;
extern int hmi_manual_page_anchor;

#endif // HMI_H
