#include "hmi_logger.h"
#include "hmi.h" // Подключаем твой главный хедер для макросов стилей ST_TEXT/ST_MUTED
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

// Инициализируем глобальный указатель
lv_obj_t *hmi_log_container = NULL;

void hmi_log_add(const char *status, const char *msg, lv_color_t status_color)
{
    // Если экран диагностики еще ни разу не создавался, аварийно выходим, чтобы не уронить MCU
    if(!hmi_log_container) return;

    // 1. ЧИТАЕМ ЖИВОЕ ВРЕМЯ ИЗ АППАРАТНОГО RTC С БАТАРЕЙКОЙ
    char time_str[12] = "00:00:00";
    time_t now;
    struct tm timeinfo;
    
    time(&now);
    localtime_r(&now, &timeinfo);
    snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", 
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    // 2. ЖЕСТКИЙ КОНТРОЛЬ ПАМЯТИ В КУЧЕ (HEAP) ДЛЯ ЭКРАНА 800х480
    // Если строк больше 40, бесследно удаляем из ОЗУ самую старую верхнюю запись
    uint32_t child_cnt = lv_obj_get_child_cnt(hmi_log_container);
    if(child_cnt >= 40) {
        lv_obj_t * oldest_row = lv_obj_get_child(hmi_log_container, 0);
        if(oldest_row) {
            lv_obj_delete(oldest_row);
        }
    }

    // 3. СБОРКА СТРОКИ ЛОГА В ОДИН ГОРИЗОНТАЛЬНЫЙ FLEX-РЯД
    lv_obj_t *row = lv_obj_create(hmi_log_container);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_layout(row, LV_LAYOUT_FLEX, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(row, 8, 0);

    // Маркер-точка (•)
    lv_obj_t *dot = lv_label_create(row);
    lv_obj_add_style(dot, ST_TEXT, 0);
    lv_label_set_text(dot, "•");
    lv_obj_set_style_text_color(dot, status_color, 0);

    // Тег статуса
    lv_obj_t *lbl_status = lv_label_create(row);
    lv_obj_add_style(lbl_status, ST_TEXT, 0);
    lv_label_set_text(lbl_status, status);
    lv_obj_set_style_text_color(lbl_status, status_color, 0);

    // Время с RTC
    lv_obj_t *lbl_time = lv_label_create(row);
    lv_obj_add_style(lbl_time, ST_MUTED, 0);
    lv_label_set_text(lbl_time, time_str);
    lv_obj_set_style_text_font(lbl_time, lv_font_16, 0);

    // Сообщение
    lv_obj_t *lbl_msg = lv_label_create(row);
    lv_obj_add_style(lbl_msg, ST_TEXT, 0);
    lv_label_set_text(lbl_msg, msg);
    lv_obj_set_style_text_font(lbl_msg, lv_font_16, 0);
    lv_label_set_long_mode(lbl_msg, LV_LABEL_LONG_WRAP); // Чистый безопасный перенос
    lv_obj_set_flex_grow(lbl_msg, 1);

    // 4. МГНОВЕННЫЙ АВТОСКРОЛЛ ДЛЯ КОНТРОЛЛЕРА
    lv_obj_update_layout(row);
    lv_obj_scroll_to_view(row, LV_ANIM_ON);
}
