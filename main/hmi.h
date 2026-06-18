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
extern lv_obj_t *manual_msg_label;
extern lv_obj_t *auto_start_btn;
extern lv_obj_t *manual_start_btn;

// Структура-"интерфейс" для вызова static функций
typedef struct {
    void (*settings_event)(lv_event_t *e);
    void (*pressure_timer_cb)(lv_timer_t *timer);
    void (*clock_timer_cb)(lv_timer_t *timer);
} hmi_events_t;

// Глобальный доступ к структуре событий
extern const hmi_events_t hmi_events;

// Внешние функции отрисовки
extern void init_fonts(uint8_t *font_buf, long buffer_size);
extern void init_styles(void);
// extern void create_auto_page(lv_obj_t *parent);
extern void create_manual_page(lv_obj_t *parent);
extern void create_setpoint_modal(lv_obj_t *parent);
extern void create_manual_confirm_modal(lv_obj_t *parent);
extern void update_pressure_ui(void);
extern void update_big_button(lv_obj_t *btn, bool state);
extern void set_message(lv_obj_t *label_obj, const char *text);
extern lv_obj_t * label(lv_obj_t * parent, const char * text, const lv_style_t * style);
extern lv_obj_t * button(lv_obj_t * parent, const char * text, const lv_style_t * style);

void hmi_create(uint8_t *font_buf, long buffer_size);

#endif // HMI_H
