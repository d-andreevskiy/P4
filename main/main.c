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

#define TAG_SPIFFS "SPIFFS_INIT"

#define TAG "main"
lv_font_t * my_font_32 = NULL;

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

// Функция создания виджетов на экране
void create_lvgl_example_widgets(void)
{
    /* 1. Создаем контейнер-основу для выравнивания */
    lv_obj_t * scr = lv_screen_active(); // В LVGL v9 вместо lv_scr_act() используется lv_screen_active()

    /* 2. Создаем кнопку */
    lv_obj_t * btn = lv_button_create(scr); // В LVGL v9 вместо lv_btn_create() используется lv_button_create()
    lv_obj_set_size(btn, 200, 60);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, -40); // Центрируем со смещением вверх
    
    // Назначаем обработчик событий на кнопку
    lv_obj_add_event_cb(btn, button_event_cb, LV_EVENT_CLICKED, NULL);

    /* 3. Добавляем текст внутрь кнопки */
    lv_obj_t * label_btn = lv_label_create(btn);
    lv_label_set_text(label_btn, "Нажми меня!");
    lv_obj_center(label_btn);

    /* 4. Создаем текстовую метку под кнопкой для отображения счетчика */
    label_count = lv_label_create(scr);
    lv_label_set_text(label_count, "Нажато раз: 0");
    // Увеличим шрифт для наглядности (используем встроенный)
    //lv_obj_set_style_text_font(label_count, &lv_font_montserrat_18, 0); 
    lv_obj_align_to(label_count, btn, LV_ALIGN_OUT_BOTTOM_MID, 0, 20); // Размещаем строго под кнопкой
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

lv_font_t * load_ttf_from_spiffs_to_ram(const char * filepath, int32_t font_size)
{
    // 1. Открываем файл обычными средствами Си
    FILE *f = fopen(filepath, "rb");
    if (f == NULL) {
        printf("Ошибка: не удалось открыть файл %s\n", filepath);
        return NULL;
    }

    // 2. Узнаем точный размер файла в байтах
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    // 3. Выделяем буфер в оперативной памяти (PSRAM)
    uint8_t *font_buffer = malloc(size);
    if (font_buffer == NULL) {
        printf("Ошибка: не хватило RAM для буфера шрифта!\n");
        fclose(f);
        return NULL;
    }

    // 4. Считываем весь файл в буфер за один проход и закрываем его
    fread(font_buffer, 1, size, f);
    fclose(f);

    // 5. Создаем шрифт из БУФЕРА В ПАМЯТИ, а не из файла
    // Передаем: путь (NULL), указатель на буфер, его размер, размер шрифта, кернинг, размер кэша глифов
    lv_font_t * my_font = lv_tiny_ttf_create_data_ex( font_buffer, size, font_size, LV_FONT_KERNING_NORMAL, 512);

    return my_font;
}

void lvgl_set_default_font(void)
{
    // 1. Создаем шрифт динамически из файла через префикс диска "S:"
    // Аргументы v9: Путь, Размер в пикселях (px), Количество глифов в кэше RAM
    lv_font_t * my_vector_font = load_ttf_from_spiffs_to_ram("/spiffs/font.ttf", 28);
    
    if (my_vector_font == NULL) {
        ESP_LOGE(TAG, "Критическая ошибка: не удалось загрузить TTF шрифт!");
        return;
    }
    ESP_LOGI(TAG, "Векторный шрифт 28px успешно создан в оперативной памяти.");


     // 1. Получаем указатель на дефолтный дисплей системы
     lv_display_t * disp = lv_display_get_default();
     if (disp == NULL) {
         ESP_LOGE(TAG, "Ошибка: активный дисплей LVGL не найден!");
         return;
     }
 
     // 2. Инициализируем стандартную тему оформления LVGL v9.x,
     // подставляя наш векторный шрифт во все три категории текстовых размеров.
     lv_theme_t * th = lv_theme_default_init(
         disp, 
         lv_palette_main(LV_PALETTE_BLUE),   // Основной цвет элементов (например, кнопок)
         lv_palette_main(LV_PALETTE_CYAN),   // Вторичный акцентный цвет темы
         false,                              // Режим отображения: false = светлая тема, true = темная тема
         my_vector_font                           // Маленький шрифт по умолчанию
     );
 
     if (th != NULL) {
         // 3. Назначаем созданную тему дисплею
         lv_display_set_theme(disp, th);
         ESP_LOGI(TAG, "Русский векторный шрифт успешно установлен по умолчанию!");
     } else {
         ESP_LOGE(TAG, "Не удалось инициализировать тему оформления.");
     }
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

void app_main(void)
{
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_DRAW_BUFF_SIZE,
        .double_buffer = BSP_LCD_DRAW_BUFF_DOUBLE,
        .flags = {
            .buff_dma = true,
            .buff_spiram = false,
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

    // lv_demo_music();
    // lv_demo_benchmark();
    // lv_demo_widgets();

    init_spiffs(); 
    lvgl_set_default_font();


    create_lvgl_example_widgets();

    bsp_display_unlock();
}