#include "hmi.h"

// Прототип локальной функции, чтобы структура увидела её адрес
static void init_styles(void);

// Создаем глобальный экземпляр структуры и сразу привязываем функцию к полю .init
hmi_styles_t hmi_styles = {
    .init = init_styles
};

static void init_styles(void)
{
    lv_style_init(ST_ROOT);
    lv_style_set_bg_color(ST_ROOT, C_BG);
    lv_style_set_bg_opa(ST_ROOT, LV_OPA_COVER);
    lv_style_set_text_color(ST_ROOT, C_TEXT);

    lv_style_init(ST_TOPBAR);
    lv_style_set_bg_color(ST_TOPBAR, C_TOP);
    lv_style_set_bg_opa(ST_TOPBAR, LV_OPA_COVER);
    lv_style_set_border_width(ST_TOPBAR, 2);
    lv_style_set_border_color(ST_TOPBAR, lv_color_hex(0x1F2B36));
    lv_style_set_pad_all(ST_TOPBAR, 16);

    lv_style_init(ST_PANEL);
    lv_style_set_bg_color(ST_PANEL, C_PANEL);
    lv_style_set_bg_opa(ST_PANEL, LV_OPA_COVER);
    lv_style_set_border_width(ST_PANEL, 1);
    lv_style_set_border_color(ST_PANEL, C_BORDER);
    lv_style_set_radius(ST_PANEL, 10);
    lv_style_set_pad_all(ST_PANEL, 18);
    lv_style_set_shadow_width(ST_PANEL, 18);
    lv_style_set_shadow_opa(ST_PANEL, LV_OPA_40);

    lv_style_init(ST_PANEL_CLICK);
    lv_style_set_border_color(ST_PANEL_CLICK, C_BLUE);

    lv_style_init(ST_TEXT);
    lv_style_set_text_color(ST_TEXT, C_TEXT);
    lv_style_set_text_font(ST_TEXT, lv_font_20);

    lv_style_init(ST_MUTED);
    lv_style_set_text_color(ST_MUTED, C_MUTED);
    lv_style_set_text_font(ST_MUTED, lv_font_22);

    lv_style_init(ST_BIG_GREEN);
    lv_style_set_text_color(ST_BIG_GREEN, C_GREEN);
    lv_style_set_text_font(ST_BIG_GREEN, lv_font_48);

    lv_style_init(ST_BIG_BLUE);
    lv_style_set_text_color(ST_BIG_BLUE, C_BLUE);
    lv_style_set_text_font(ST_BIG_BLUE, lv_font_48);

    lv_style_init(ST_BIG_ORANGE);
    lv_style_set_text_color(ST_BIG_ORANGE, C_ORANGE);
    lv_style_set_text_font(ST_BIG_ORANGE, lv_font_48);

    lv_style_init(ST_BTN_DARK);
    lv_style_set_bg_color(ST_BTN_DARK, C_PANEL_2);
    lv_style_set_bg_opa(ST_BTN_DARK, LV_OPA_COVER);
    lv_style_set_border_width(ST_BTN_DARK, 1);
    lv_style_set_border_color(ST_BTN_DARK, C_BORDER_LT);
    lv_style_set_radius(ST_BTN_DARK, 8);
    lv_style_set_pad_all(ST_BTN_DARK, 12);

    lv_style_init(ST_BTN_GREEN);
    lv_style_set_bg_color(ST_BTN_GREEN, lv_color_hex(0x1F9A3A));
    lv_style_set_bg_grad_color(ST_BTN_GREEN, lv_color_hex(0x39C95A));
    lv_style_set_bg_grad_dir(ST_BTN_GREEN, LV_GRAD_DIR_VER);
    lv_style_set_radius(ST_BTN_GREEN, 10);

    lv_style_init(ST_BTN_RED);
    lv_style_set_bg_color(ST_BTN_RED, lv_color_hex(0xB91515));
    lv_style_set_bg_grad_color(ST_BTN_RED, lv_color_hex(0xF33434));
    lv_style_set_bg_grad_dir(ST_BTN_RED, LV_GRAD_DIR_VER);
    lv_style_set_radius(ST_BTN_RED, 10);

    lv_style_init(ST_BTN_BLUE);
    lv_style_set_bg_color(ST_BTN_BLUE, lv_color_hex(0x1662C8));
    lv_style_set_bg_grad_color(ST_BTN_BLUE, lv_color_hex(0x339CFF));
    lv_style_set_bg_grad_dir(ST_BTN_BLUE, LV_GRAD_DIR_VER);
    lv_style_set_radius(ST_BTN_BLUE, 8);

    lv_style_init(ST_BTN_GRAY);
    lv_style_set_bg_color(ST_BTN_GRAY, lv_color_hex(0x1C2732));
    lv_style_set_bg_grad_color(ST_BTN_GRAY, lv_color_hex(0x334250));
    lv_style_set_bg_grad_dir(ST_BTN_GRAY, LV_GRAD_DIR_VER);


      // Настройка фона подложки клавиатуры
      lv_style_init(&hmi_styles.st_kb_main);
      lv_style_set_bg_color(&hmi_styles.st_kb_main, lv_color_hex(0x151E27)); 
      lv_style_set_bg_opa(&hmi_styles.st_kb_main, LV_OPA_COVER);
      lv_style_set_pad_all(&hmi_styles.st_kb_main, 8);
      lv_style_set_pad_gap(&hmi_styles.st_kb_main, 6);
      lv_style_set_border_width(&hmi_styles.st_kb_main, 0);
  
      // Настройка кнопок клавиатуры (Обычное состояние)
      lv_style_init(&hmi_styles.st_kb_items);
      lv_style_set_bg_color(&hmi_styles.st_kb_items, lv_color_hex(0x212D3A)); // Твой глубокий сине-серый
      lv_style_set_bg_opa(&hmi_styles.st_kb_items, LV_OPA_COVER);
      lv_style_set_text_color(&hmi_styles.st_kb_items, lv_color_white());     
      lv_style_set_radius(&hmi_styles.st_kb_items, 6);                        
      
    // Найти в hmi_styles.c блок настройки шрифта для клавиатуры и заменить:

       // Найти блок st_kb_items в hmi_styles.c и заменить:
       lv_style_init(&hmi_styles.st_kb_items);
       lv_style_set_bg_color(&hmi_styles.st_kb_items, lv_color_hex(0x212D3A)); // Твой сине-серый
       lv_style_set_bg_opa(&hmi_styles.st_kb_items, LV_OPA_COVER);
       lv_style_set_text_color(&hmi_styles.st_kb_items, lv_color_white());     
       lv_style_set_radius(&hmi_styles.st_kb_items, 6);                        
       
       // ЖЕЛЕЗНЫЙ ВАРИАНТ: Передаем адрес твоего инициализированного шрифта напрямую.
       // (Подставь имя своего шрифта, который ты используешь для вывода обычного текста в hmi.c)
       lv_style_set_text_font(&hmi_styles.st_kb_items, &lv_font_montserrat_20); 
   
       lv_style_set_border_width(&hmi_styles.st_kb_items, 0);   
       lv_style_set_shadow_width(&hmi_styles.st_kb_items, 0);   
       lv_style_set_outline_width(&hmi_styles.st_kb_items, 0);  
   
  

    


}

__attribute__((constructor)) static void register_styles_api(void) {
    hmi_styles.init = init_styles;
}