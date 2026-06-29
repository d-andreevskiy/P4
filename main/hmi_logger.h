#ifndef HMI_LOGGER_H
#define HMI_LOGGER_H

#include "lvgl.h"

// Глобальный указатель на контейнер логов, который hmi_settings.c инициализирует при сборке экрана
extern lv_obj_t *hmi_log_container;

/**
 * @brief Добавить системную запись в графический журнал на экране контроллера
 * @param status Тег статуса строки ("OK", "WARN", "ERR")
 * @param msg Текст сообщения лога
 * @param status_color Цвет маркера и тега статуса
 */
void hmi_log_add(const char *status, const char *msg, lv_color_t status_color);

#endif // HMI_LOGGER_H
