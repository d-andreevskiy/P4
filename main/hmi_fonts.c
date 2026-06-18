#include "hmi.h"

lv_font_t *lv_font_20;
lv_font_t *lv_font_22;
lv_font_t *lv_font_24;
lv_font_t *lv_font_32;
lv_font_t *lv_font_36;
lv_font_t *lv_font_48;

#define TAG "HMI_FONTS"
#include <esp_log.h>

// Ваша рабочая функция считывания файла со SPIFFS в PSRAM (оставляем без изменений)
long font_to_buf(const char *filepath, uint8_t **font_buffer)
{
    FILE *f = fopen(filepath, "rb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Ошибка: не удалось открыть файл %s", filepath);
        return 0;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0) {
        ESP_LOGE(TAG, "Ошибка: файл %s пустой!", filepath);
        fclose(f);
        return 0;
    }

    // Переносим буфер во внутреннюю быструю память SRAM
    *font_buffer = (uint8_t *)heap_caps_aligned_alloc(64, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    // *font_buffer = (uint8_t *)heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (*font_buffer == NULL) {
        ESP_LOGE(TAG, "Критическая ошибка: не хватило PSRAM (%ld байт) для шрифта!", size);
        fclose(f);
        return 0;
    }

    fread(*font_buffer, 1, size, f);
    fclose(f);



    ESP_LOGI(TAG, "Файл %s успешно считан в PSRAM. Размер: %ld байт.", filepath, size);
    return size;
}

lv_font_t * lvgl_font_init(const uint8_t * ttf_data, uint32_t data_size, int32_t font_size)
{
    ESP_LOGI("UI", "Создание Tiny TrueType шрифта напрямую из памяти для %d px...", font_size);

    // Вызываем официальный конструктор Tiny TrueType из памяти.
    // Передаем: указатель на PSRAM, размер массива байт и целевую высоту шрифта.
    lv_font_t * font = lv_tiny_ttf_create_data(ttf_data, data_size, font_size);

    if (font == NULL) {
        ESP_LOGE("UI", "КРИТИЧЕСКАЯ ОШИБКА: Tiny TrueType не смог обработать массив шрифта %d px!", font_size);
        return NULL;
    }

    ESP_LOGI("UI", "УСПЕХ: Векторный шрифт Tiny TTF %d px успешно создан!", font_size);

    // Настройка фолбеков Montserrat
    switch (font_size) {
        case 14:
        case 16: font->fallback = &lv_font_montserrat_16; break;
        case 18: font->fallback = &lv_font_montserrat_18; break;
        case 20:
        case 22:
        case 24: font->fallback = &lv_font_montserrat_24; break;
        case 30:
        case 32: font->fallback = &lv_font_montserrat_32; break;
        case 36: font->fallback = &lv_font_montserrat_36; break;
        case 48: font->fallback = &lv_font_montserrat_48; break;
        default: font->fallback = &lv_font_montserrat_16; break;
    }

    return font;
}

void init_fonts(uint8_t *font_buf, long buffer_size)
{
 

    // long buffer_size = font_to_buf("/spiffs/font.ttf", &font_buf);

    if (buffer_size <= 0 || font_buf == NULL) {
        ESP_LOGE(TAG, "Не удалось продолжить: буфер шрифта пуст.");
        return;
    }

    ESP_LOGI(TAG, "Мгновенное создание шрифтов из выделенной PSRAM...");
    
    lv_font_20 = lvgl_font_init(font_buf, buffer_size, 20);
    lv_font_22 = lvgl_font_init(font_buf, buffer_size, 22);
    lv_font_24 = lvgl_font_init(font_buf, buffer_size, 24);
    lv_font_32 = lvgl_font_init(font_buf, buffer_size, 32);
    lv_font_36 = lvgl_font_init(font_buf, buffer_size, 36);
    lv_font_48 = lvgl_font_init(font_buf, buffer_size, 48);
}
