
idf_component_register(
    SRCS "esp32_p4_wifi6_touch_lcd_7b.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "priv_include"
    REQUIRES 
        driver esp_driver_gpio
        sdmmc
        esp_driver_sdmmc  
        esp_lcd 
        usb spiffs
        fatfs  
        esp_driver_i2s 
        esp_driver_i2c 
        esp_driver_ledc 
        nvs_flash 
        esp_codec_dev
        espressif__esp_lvgl_port

         
        # es8311 
)

idf_build_set_property(COMPILE_OPTIONS "-DCONFIG_LV_DRAW_SW_SUPPORT_RGB565_SWAPPED=1" APPEND)
