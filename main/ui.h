#ifndef UI_H
#define UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

// Инициализация интерфейса
void hmi_create(uint8_t *font_buf, long buffer_size);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*UI_H*/
