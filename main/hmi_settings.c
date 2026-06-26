#include "hmi.h"

// Глобальные переменные навигации (объявлены в hmi.h)
lv_obj_t * settings_page = NULL;

// Внутренние статические виджеты модуля настроек
static lv_obj_t *menu_btns[5];
static lv_obj_t *sub_pages[5];
static uint8_t active_sub_page = 0;

// Элементы управления встроенным модальным Numpad
static lv_obj_t *modal_kb_bg = NULL;
static lv_obj_t *modal_ta = NULL;
static int32_t *current_editing_val = NULL; // Указатель на текущую переменную регистра
static lv_obj_t *current_val_btn = NULL;     // Кнопка, по которой кликнули

// Временный буфер для карты регистров (Modbus / NVS)
static int32_t temp_reg9 = 180;   // Мин. датчик давления
static int32_t temp_reg10 = 3850; // Макс. датчик давления
static int32_t temp_reg11 = 1;    // Номер входа давления
static int32_t temp_reg17 = 2;    // Номер входа уровня воды

// Прототипы локальных функций
static void settings_page_create(lv_obj_t *parent);
static void settings_page_show(void);
static void settings_page_hide(void);
static void build_calibration_sub_page(lv_obj_t *page);
static void build_timers_sub_page(lv_obj_t *page);
static void menu_tab_event_cb(lv_event_t *e);
static void param_input_click_cb(lv_event_t *e);
static void kb_event_cb(lv_event_t *e);
static void create_param_row(lv_obj_t *parent, const char *name, const char *desc, const char *reg_name, int32_t *val_ptr);
static void hmi_settings_save_event(lv_event_t *e);

// переменные для выделения редактируемых значений
static int button_old_index;
static lv_obj_t *button_old_parent; 

static void kb_value_changed_cb(lv_event_t * e)
{
    // Проверяем, что событие — именно изменение текста
    if(lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;

    if(!modal_ta || !current_editing_val) return;

    // Забираем то, что оператор УЖЕ успел набрать в поле ввода
    const char * text = lv_textarea_get_text(modal_ta);
    int32_t current_input = atoi(text);

    int32_t max_limit = 99999; // Значение по умолчанию, если регистр не распознан

    // Задаем жесткие потолки (максимумы) для каждого регистра
    if(current_editing_val == &temp_reg9)       max_limit = 1000;
    else if(current_editing_val == &temp_reg10) max_limit = 4095;
    else if(current_editing_val == &temp_reg11) max_limit = 7;
    else if(current_editing_val == &temp_reg17) max_limit = 31;

    // ЕСЛИ ВВЕДЕННОЕ ЧИСЛО ПРЕВЫШАЕТ МАКСИМУМ:
    if(current_input > max_limit) 
    {
        // Вызываем встроенную команду удаления последнего символа (BackSpace)
        lv_textarea_delete_char(modal_ta);
        
        // Опционально: можно заставить экран кратковременно моргнуть 
        // или издать писк бузером, сигнализируя о запрете ввода
    }
}


// --- ГЛАВНЫЙ КОНСТРУКТОР ВЕРСТКИ СТРАНИЦЫ НАСТРОЕК ---
static void settings_page_create(lv_obj_t *parent)
{
    settings_page = lv_obj_create(parent);
    lv_obj_remove_style_all(settings_page);


    lv_obj_add_style(settings_page, ST_TOPBAR, 0);

    lv_obj_set_style_border_width(settings_page,0,0);
    lv_obj_set_style_outline_width(settings_page,0,0);
    lv_obj_set_style_shadow_width(settings_page,0,0);

    lv_obj_set_size(settings_page, LV_PCT(100), LV_PCT(100));



    // Сетка: Левое меню (240px) | Контент (Остаток) ; Строки: Контент | Футер (60px)
    static lv_coord_t main_col_dsc[] = {240, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t main_row_dsc[] = {LV_GRID_FR(1), 60, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(settings_page, main_col_dsc, main_row_dsc);

    lv_obj_set_style_layout(settings_page, LV_LAYOUT_GRID, 0);

    lv_obj_set_style_pad_all(settings_page, 10, 0);
    lv_obj_set_style_pad_gap(settings_page, 15, 0);
    lv_obj_add_flag(settings_page, LV_OBJ_FLAG_HIDDEN); // По умолчанию скрыта

    // --- ЛЕВОЕ БОКОВОЕ МЕНЮ ---
    lv_obj_t *left_menu = lv_obj_create(settings_page);
    lv_obj_add_style(left_menu, ST_TOPBAR, 0);
    lv_obj_set_grid_cell(left_menu, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 2);
    lv_obj_set_flex_flow(left_menu, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(left_menu, 10, 0);
    lv_obj_set_style_pad_gap(left_menu, 10, 0);

    lv_obj_t *menu_title = label(left_menu, "НАСТРОЙКИ", ST_TEXT);
    lv_obj_set_style_margin_bottom(menu_title, 10, 0);

    const char *menu_names[] = {
        "1. Калибровка датчиков",
        "2. Таймеры и задержки",
        "3. Защита сухого хода",
        "4. Диагностика системы",
        "5. Сетевые настройки"
    };

    for(int i = 0; i < 5; i++) {
        menu_btns[i] = button(left_menu, menu_names[i], ST_BTN_DARK);
        lv_obj_set_size(menu_btns[i], LV_PCT(100), 48);
        lv_obj_set_user_data(menu_btns[i], (void*)(uintptr_t)i);
        lv_obj_add_event_cb(menu_btns[i], menu_tab_event_cb, LV_EVENT_CLICKED, NULL);
    }

    // --- ПРАВАЯ ОБЛАСТЬ КОНТЕНТА ПОДСТРАНИЦ ---
    lv_obj_t *content_area = lv_obj_create(settings_page);
    lv_obj_remove_style_all(content_area);

    lv_obj_set_style_border_width(content_area,0,0);
    lv_obj_set_style_outline_width(content_area,0,0);
    lv_obj_set_style_shadow_width(content_area,0,0);

    lv_obj_set_grid_cell(content_area, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

    for(int i = 0; i < 5; i++) {
        sub_pages[i] = lv_obj_create(content_area);
        lv_obj_remove_style_all(sub_pages[i]);

        lv_obj_set_style_border_width(sub_pages[i],0,0);
        lv_obj_set_style_outline_width(sub_pages[i],0,0);
        lv_obj_set_style_shadow_width(sub_pages[i],0,0);

        lv_obj_set_size(sub_pages[i], LV_PCT(100), LV_PCT(100));
        lv_obj_add_flag(sub_pages[i], LV_OBJ_FLAG_HIDDEN);
    }

    // Строим вкладки контента
    build_calibration_sub_page(sub_pages[0]);
    build_timers_sub_page(sub_pages[1]);

    lv_obj_clear_flag(sub_pages[0], LV_OBJ_FLAG_HIDDEN); // По умолчанию открыта первая
    active_sub_page = 0;

    // --- НИЖНИЙ ФУТЕР ---
    lv_obj_t *footer = lv_obj_create(settings_page);
    lv_obj_remove_style_all(footer);
    lv_obj_set_grid_cell(footer, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *hint = label(footer, "Редактирование параметров. После изменения нажмите \"Сохранить\".", ST_MUTED);
    
    lv_obj_t *btn_box = lv_obj_create(footer);
    lv_obj_remove_style_all(btn_box);
    lv_obj_set_flex_flow(btn_box, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(btn_box, 15, 0);

    lv_obj_t *save_btn = button(btn_box, "Сохранить", ST_BTN_GREEN);
    lv_obj_set_size(save_btn, 130, 42);
    lv_obj_add_event_cb(save_btn, hmi_settings_save_event, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_btn = button(btn_box, "Отмена", ST_BTN_RED);
    lv_obj_set_size(back_btn, 130, 42);
    lv_obj_add_event_cb(back_btn, hmi_events.settings_event, LV_EVENT_CLICKED, NULL); 
}

// Вкладка 1: Калибровка датчиков и входов
static void build_calibration_sub_page(lv_obj_t *page)
{
    lv_obj_set_flex_flow(page, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(page, 10, 0);
    lv_obj_set_style_pad_gap(page, 10, 0);
    lv_obj_add_flag(page, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *sec1 = label(page, "1.1 Датчик давления", ST_TEXT);
    lv_obj_set_style_text_color(sec1, lv_color_hex(0x3498DB), 0);

    create_param_row(page, "Мин. значение датчика давления", "ADC/сырое значение при 0 бар", "Регистр 9", &temp_reg9);
    create_param_row(page, "Макс. значение датчика давления", "ADC/сырое значение при 16 бар", "Регистр 10", &temp_reg10);
    create_param_row(page, "Номер входа датчика давления", "Канал аналогового входа", "Регистр 11", &temp_reg11);

    lv_obj_t *sec2 = label(page, "1.2 Датчик уровня воды", ST_TEXT);
    lv_obj_set_style_text_color(sec2, lv_color_hex(0x3498DB), 0);
    lv_obj_set_style_margin_top(sec2, 10, 0);

    create_param_row(page, "Номер входа датчика уровня воды", "Дискретный вход релейного датчика", "Регистр 17", &temp_reg17);
}

// Вкладка 2: Таймеры (заготовка для следующих шагов)
static void build_timers_sub_page(lv_obj_t *page)
{
    lv_obj_set_flex_flow(page, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(page, 10, 0);
    lv_obj_set_style_pad_gap(page, 10, 0);
    lv_obj_add_flag(page, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *sec = label(page, "Временные параметры системы", ST_TEXT);
    lv_obj_set_style_text_color(sec, lv_color_hex(0x3498DB), 0);
}

static void create_param_row(lv_obj_t *parent, const char *name, const char *desc, const char *reg_name, int32_t *val_ptr)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), 60);
    lv_obj_add_style(row, ST_TOPBAR, 0);
    
    // 1. ЖЕСТКО ОБНУЛЯЕМ ОТСТУПЫ СТРОКИ: убираем любые скрытые пиксели темы
    lv_obj_set_style_pad_all(row, 0, 0); 
    lv_obj_set_style_pad_hor(row, 15, 0);
    lv_obj_set_style_margin_all(row, 0, 0);

    static lv_coord_t r_cols[] = {LV_GRID_FR(3), LV_GRID_FR(1), 100, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t r_rows[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    
    lv_obj_set_grid_dsc_array(row, r_cols, r_rows);
    lv_obj_set_style_layout(row, LV_LAYOUT_GRID, 0); 

    // Текстовый контейнер (Имя + Описание)
    lv_obj_t *txt_box = lv_obj_create(row);
    lv_obj_remove_style_all(txt_box);
    lv_obj_set_style_layout(txt_box, LV_LAYOUT_FLEX, 0); 
    lv_obj_set_flex_flow(txt_box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(txt_box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    
    // Зажимаем его в ячейку (0,0) и ограничиваем высоту, чтобы он не выдавливал строку вниз
    lv_obj_set_grid_cell(txt_box, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_size(txt_box, LV_PCT(100), 50); // Высота чуть меньше строки (50px при строке 60px)
    lv_obj_set_style_pad_all(txt_box, 0, 0); 
    
    label(txt_box, name, ST_TEXT);
    lv_obj_t *lbl_desc = label(txt_box, desc, ST_MUTED);
    lv_obj_set_style_text_font(lbl_desc, lv_font_16, 0); 

    // Метка регистра (Регистр 9, 10...)
    lv_obj_t *lbl_reg = label(row, reg_name, ST_TEXT);
    lv_obj_set_style_text_color(lbl_reg, lv_color_hex(0xE67E22), 0); 
    // Зажимаем в ячейку (1,0) и гасим внешние маргины
    lv_obj_set_grid_cell(lbl_reg, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_style_margin_all(lbl_reg, 0, 0);

    // Кнопка со значением
    char char_buf[16];
    snprintf(char_buf, sizeof(char_buf), "%d", *val_ptr);
    lv_obj_t *val_btn = button(row, char_buf, ST_BTN_DARK);
    lv_obj_add_style(val_btn, ST_BTN_SELECTED, LV_STATE_USER_1);
    lv_obj_set_size(val_btn, 90, 38);
    // Зажимаем в ячейку (2,0)
    lv_obj_set_grid_cell(val_btn, LV_GRID_ALIGN_END, 2, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_style_margin_all(val_btn, 0, 0);
    
    lv_obj_set_user_data(val_btn, val_ptr);
    lv_obj_add_event_cb(val_btn, param_input_click_cb, LV_EVENT_CLICKED, NULL);
}


// --- ЛОГИКА СИСТЕМНОГО NUMPAD КЛИКА ---
static void param_input_click_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    current_val_btn = lv_event_get_target(e);
    current_editing_val = (int32_t *)lv_obj_get_user_data(current_val_btn);


    // 1. Создаем блокирующую подложку на самом верхнем системном слое (на весь экран)
    modal_kb_bg = lv_obj_create(lv_screen_active());
    lv_obj_set_size(modal_kb_bg, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(modal_kb_bg, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(modal_kb_bg, LV_OPA_60, 0);
    lv_obj_remove_flag(modal_kb_bg, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_move_to_index(current_val_btn, -1);
    lv_obj_add_state(current_val_btn, LV_STATE_USER_1);

    lv_obj_set_style_outline_width(modal_kb_bg,0,0);
    lv_obj_set_style_border_width(modal_kb_bg,0,0);

    // 3. Создаем контейнер клавиатуры внутри подложки
    lv_obj_t *box = lv_obj_create(modal_kb_bg);
    lv_obj_set_size(box, 380, 350);
    lv_obj_center(box);
    lv_obj_add_style(box, ST_TOPBAR, 0);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(box, 12, 0);
    lv_obj_set_style_pad_gap(box, 10, 0);
    lv_obj_set_style_outline_width(box,0,0);

    // 4. Текстовое поле ввода внутри бокса
    modal_ta = lv_textarea_create(box);
    lv_obj_set_width(modal_ta, LV_PCT(100));
    lv_textarea_set_one_line(modal_ta, true);
    lv_textarea_set_accepted_chars(modal_ta, "0123456789-");
    lv_obj_add_event_cb(modal_ta, kb_value_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_remove_flag(modal_ta, LV_OBJ_FLAG_CLICK_FOCUSABLE);

    char char_buf[16];
    snprintf(char_buf, sizeof(char_buf), "%ld", (long)*current_editing_val);
    lv_textarea_set_text(modal_ta, char_buf);

    // 5. Создаем встроенную клавиатуру
    lv_obj_t *kb = lv_keyboard_create(box);
    lv_obj_set_width(kb, LV_PCT(100));
    lv_obj_set_flex_grow(kb, 1);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_NUMBER);
    lv_keyboard_set_textarea(kb, modal_ta);

    // Накладываем матовые стили на клавиатуру
    lv_obj_add_style(kb, ST_KB_MAIN, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(kb, ST_KB_ITEMS, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_add_style(kb, ST_KB_ITEMS, LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_add_style(kb, ST_KB_ITEMS_PR, LV_PART_ITEMS | LV_STATE_PRESSED);

    lv_obj_set_style_outline_width(kb,0,LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(kb,0,LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_CANCEL, NULL);

    button_old_parent = lv_obj_get_parent(current_val_btn);
    button_old_index = lv_obj_get_index(current_val_btn);

    lv_area_t btn_coords;
    lv_obj_get_coords(current_val_btn, &btn_coords);

    lv_obj_set_parent(current_val_btn, lv_screen_active());
    lv_obj_set_pos(current_val_btn, btn_coords.x1, btn_coords.y1);

    lv_obj_move_to_index(current_val_btn, -1);

    lv_obj_add_state(current_val_btn, LV_STATE_USER_1);

    lv_obj_update_layout(current_val_btn);
}

static void kb_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_READY) 
    {
        // Нажата галочка (✓) — сохраняем измененияconst 
        const char * text = lv_textarea_get_text(modal_ta);
        int32_t new_val = atoi(text);
        if(current_editing_val) *current_editing_val = new_val;
        // Динамически обновляем число на кнопке таблицы параметров
        if(current_val_btn) 
        {
            lv_obj_t * label_obj = lv_obj_get_child(current_val_btn, 0);
            if(label_obj) lv_label_set_text(label_obj, text);
        }
        
        lv_obj_delete(modal_kb_bg);
        modal_kb_bg = NULL;
    }
    else if(code == LV_EVENT_CANCEL) 
    {
        // Нажат крестик (X) — отмена ввода
        lv_obj_delete(modal_kb_bg);
        modal_kb_bg = NULL;
    }

    if (current_val_btn && button_old_parent) {
        lv_obj_remove_state(current_val_btn, LV_STATE_USER_1);
        lv_obj_set_parent(current_val_btn, button_old_parent);
        lv_obj_move_to_index(current_val_btn, button_old_index);
        lv_obj_set_grid_cell(current_val_btn, LV_GRID_ALIGN_END, 2,1, LV_GRID_ALIGN_CENTER, 0,1);
        lv_obj_update_layout(current_val_btn);
    }

}

static void menu_tab_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) 
        return;
        
    lv_obj_t *btn = lv_event_get_target(e);
    uintptr_t target_idx = (uintptr_t)lv_obj_get_user_data(btn);
    lv_obj_add_flag(sub_pages[active_sub_page], LV_OBJ_FLAG_HIDDEN);
    active_sub_page = target_idx;
    lv_obj_clear_flag(sub_pages[active_sub_page], LV_OBJ_FLAG_HIDDEN);
}

// Заглушки для глобальных регистров (чтобы связать их с Modbus/NVS)
int32_t modbus_reg9 = 180;
int32_t modbus_reg10 = 3850;
int32_t modbus_reg11 = 1;
int32_t modbus_reg17 = 2;

static void settings_page_show(void){
    if (settings_page) {
        // Подтягиваем актуальные боевые значения перед редактированием
        temp_reg9  = modbus_reg9;
        temp_reg10 = modbus_reg10;
        temp_reg11 = modbus_reg11;
        temp_reg17 = modbus_reg17;
        
        lv_obj_clear_flag(settings_page, LV_OBJ_FLAG_HIDDEN);
        hmi_current_page = HMI_PAGE_SETTINGS;
    }
}

static void settings_page_hide(void)
{
    if (settings_page)
    {
        lv_obj_add_flag(settings_page, LV_OBJ_FLAG_HIDDEN);
    }
}

static void hmi_settings_save_event(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED)
        return;

    // Переносим измененные данные из временного буфера в боевые регистры прошивки
    modbus_reg9  = temp_reg9;
    modbus_reg10 = temp_reg10;
    modbus_reg11 = temp_reg11;
    modbus_reg17 = temp_reg17;
    
    // Вызываем имитацию клика кнопки Назад для закрытия страницы
    hmi_events.settings_event(e);
}

// --- АВТОРЕГИСТРАЦИЯ API НАСТРОЕК ---
struct hmi_settings hmi_settings;

__attribute__((constructor)) static void register_settings_page_api(void) 
{
    hmi_settings.create = settings_page_create;
    hmi_settings.show   = settings_page_show;
    hmi_settings.hide   = settings_page_hide;
}