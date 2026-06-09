/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_memory_utils.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "bsp_board_extra.h"
#include "lv_demos.h"

#include "lvgl.h"
#include "esp_spiffs.h"

#include "sys/stat.h"
#include "libs/tiny_ttf/lv_tiny_ttf.h"

#include "ui.h"

#define TAG_SPIFFS "SPIFFS_INIT"

#define TAG "main"

// Переменная для хранения объекта текстовой метки
static lv_obj_t * label_count;
static uint8_t counter = 0;

// Функция обработки нажатия на кнопку
static void button_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        counter++;
        // Обновляем текст на метке с новым значением счетчика
        lv_label_set_text_fmt(label_count, "Нажато раз: %d", counter);
    }
}

bool spiffs_file_exists(const char *filepath)
{
    struct stat st;
    
    // Функция stat возвращает 0, если файл найден и к нему есть доступ
    if (stat(filepath, &st) == 0) {
        ESP_LOGI(TAG_SPIFFS, "Файл найден: %s (%ld байт)", filepath, st.st_size);
        return true;
    }
    
    ESP_LOGW(TAG_SPIFFS, "Файл НЕ существует: %s", filepath);
    return false;
}

esp_err_t init_spiffs(void)
{
    ESP_LOGI(TAG_SPIFFS, "Инициализация файловой системы SPIFFS...");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",         // Точка монтирования в файловой системе C
        .partition_label = "storage",    // Имя раздела из вашей таблицы partitions.csv
        .max_files = 5,                 // Максимальное количество одновременно открытых файлов
        .format_if_mount_failed = true  // Форматировать раздел, если он пуст или поврежден
    };

    // Регистрируем и монтируем SPIFFS
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG_SPIFFS, "Ошибка: не удалось смонтировать или отформатировать файловую систему");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG_SPIFFS, "Ошибка: раздел с именем '%s' не найден в partitions.csv", conf.partition_label);
        } else {
            ESP_LOGE(TAG_SPIFFS, "Ошибка инициализации SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    // Проверяем работоспособность и выводим объем памяти
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG_SPIFFS, "SPIFFS успешно смонтирован!");
        ESP_LOGI(TAG_SPIFFS, "Размер раздела: %d КБ, Использовано: %d КБ", total / 1024, used / 1024);
    } else {
        ESP_LOGE(TAG_SPIFFS, "Не удалось получить информацию о разделе (%s)", esp_err_to_name(ret));
    }

    return ESP_OK;
}


extern long font_to_buf(const char *filepath, uint8_t **font_buffer);

void app_main(void)
{
    init_spiffs(); 

    uint8_t *font_buf = NULL;
    long buffer_size = font_to_buf("/spiffs/font.ttf", &font_buf);


    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_DRAW_BUFF_SIZE,
        .double_buffer = BSP_LCD_DRAW_BUFF_DOUBLE,
        .flags = {
            .buff_dma = true,
            .buff_spiram = true,
            .sw_rotate = true,
        }
    };
    lv_display_t *disp = bsp_display_start_with_config(&cfg);

    bsp_display_backlight_on();

    if (disp != NULL)
    {
        bsp_display_rotate(disp, LV_DISPLAY_ROTATION_180);
    }

    bsp_display_lock(0);



    hmi_create(font_buf, buffer_size);

    bsp_display_unlock();
}