#include "hmi.h"
#include "esp_log.h"

static const char *TAG = "HMI_EVENTS";

// Временные переменные для логики уставки (если они не объявлены глобально)
float set_pressure_old = 5.0f;

// --- ПРОТОТИПЫ ЛОКАЛЬНЫХ ФУНКЦИЙ ---
static void settings_event(lv_event_t *e);
static void pressure_timer_cb(lv_timer_t *timer);
static void clock_timer_cb(lv_timer_t *timer);
static void open_setpoint_event(lv_event_t *e);
static void close_setpoint_event(lv_event_t *e);
static void save_setpoint_event(lv_event_t *e);
static void pressure_minus_event(lv_event_t *e);
static void pressure_plus_event(lv_event_t *e);
static void to_manual_event(lv_event_t *e);
static void auto_start_event(lv_event_t *e);
static void confirm_manual_event(lv_event_t *e);
static void cancel_manual_event(lv_event_t *e);

// --- 1. СОБЫТИЯ ТОПБАРА И НАВИГАЦИИ ---
static void settings_event(lv_event_t *e)
{
    LV_UNUSED(e);
    if (auto_screen_active)
        set_message(auto_msg_label, "Открытие настроек пока не реализовано");
    else
        hmi_manual_page.set_message( "Открытие настроек пока не реализовано");
}

static void to_manual_event(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
        hmi_confirm_modal_api.show();

    // {
    //     hmi_auto_page.hide();
    //     hmi_manual_page.show();
    //     lv_label_set_text(screen_title, "РУЧНОЙ РЕЖИМ");
    // }
}

static void confirm_manual_event(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        hmi_confirm_modal_api.hide();
        hmi_auto_page.hide();
        hmi_manual_page.show();

        auto_running = false;
        update_big_button(auto_start_btn, false);
        auto_screen_active = false;
        
        lv_label_set_text(screen_title, "РУЧНОЙ РЕЖИМ");
        hmi_manual_page.set_message("Переключено в ручной режим");
    }
}

static void cancel_manual_event(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        hmi_confirm_modal_api.hide();
    }
}


// --- 2. СИСТЕМНЫЕ ТАЙМЕРЫ ---
static void pressure_timer_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);
    if (auto_running)
    {
        actual_pressure += (set_pressure - actual_pressure) * 0.07f;
    }
    else
    {
        actual_pressure += 0.01f;
        if (actual_pressure > 8.1f)
            actual_pressure = 7.7f;
    }

    if (actual_pressure < 0.0f) actual_pressure = 0.0f;
    if (actual_pressure > HMI_PRESSURE_MAX) actual_pressure = HMI_PRESSURE_MAX;

    // Вызываем инкапсулированное обновление активной страницы
    if (auto_screen_active) {
        hmi_auto_page.update();
    } else {
       // hmi_manual_page.update(); // Если ручной режим поддерживает .update
    }
}

static void open_auto_event(lv_event_t *e)
{
    LV_UNUSED(e);
    
    // Сбрасываем ручной режим на уровне логики прошивки
    manual_running = false;
    hmi_manual_page.update_status(false);

    // Переключаем экраны через API модулей
    auto_screen_active = true;
    lv_label_set_text(screen_title, "АВТО РЕЖИМ");
    
    hmi_manual_page.hide();
    hmi_auto_page.show(); // Чистый вызов вместо lv_obj_clear_flag
    
    // Выводим уведомление на главном экране автоматики
    set_message(auto_msg_label, "Авто режим активирован");
}

static void clock_timer_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);
    static int minute = 30;
    static int hour = 12;
    static int tick = 0;
    char buf[12];

    tick++;
    if (tick >= 60)
    {
        tick = 0;
        minute++;
        if (minute >= 60)
        {
            minute = 0;
            hour = (hour + 1) % 24;
        }
    }

    snprintf(buf, sizeof(buf), "%02d:%02d", hour, minute);
    lv_label_set_text(clock_label, buf);
}

// --- 3. ЛОГИКА УПРАВЛЕНИЯ МОДАЛЬНЫМ ОКНОМ УСТАВКИ ---
static void open_setpoint_event(lv_event_t *e)
{
    LV_UNUSED(e);
    hmi_setpoint_modal_api.show();
    ESP_LOGI(TAG, "%d: clicked", __LINE__);
    set_pressure_old = set_pressure;
}

static void close_setpoint_event(lv_event_t *e)
{
    LV_UNUSED(e);
    hmi_setpoint_modal_api.hide();
    set_pressure = set_pressure_old;
    hmi_auto_page.update();
}

static void save_setpoint_event(lv_event_t *e)
{
    LV_UNUSED(e);
    hmi_setpoint_modal_api.hide();
    // Фиксируем новую уставку и обновляем интерфейс
    hmi_auto_page.update();
}

static void pressure_minus_event(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        set_pressure -= 0.1f;
        if (set_pressure < 0.0f) set_pressure = 0.0f;
        hmi_auto_page.update(); // Обновляет и карточку, и обертку внутри модалки
    }
}

static void pressure_plus_event(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        set_pressure += 0.1f;
        if (set_pressure > HMI_PRESSURE_MAX) set_pressure = HMI_PRESSURE_MAX;
        hmi_auto_page.update();
    }
}

static void auto_start_event(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        auto_running = !auto_running;
        update_big_button(auto_start_btn, auto_running);
    }
}



// --- АВТОРЕГИСТРАЦИЯ API СОБЫТИЙ ---
struct hmi_events_t hmi_events;

__attribute__((constructor)) static void register_events_api(void) {
    hmi_events.settings_event      = settings_event;
    hmi_events.pressure_timer_cb   = pressure_timer_cb;
    hmi_events.clock_timer_cb      = clock_timer_cb;
    hmi_events.open_setpoint_event = open_setpoint_event;
    hmi_events.close_setpoint_event= close_setpoint_event;
    hmi_events.save_setpoint_event = save_setpoint_event;
    hmi_events.pressure_minus_event= pressure_minus_event;
    hmi_events.pressure_plus_event = pressure_plus_event;
    hmi_events.to_manual_event     = to_manual_event;
    hmi_events.auto_start_event    = auto_start_event;
    hmi_events.confirm_manual_event = confirm_manual_event;
    hmi_events.cancel_manual_event  = cancel_manual_event;
    hmi_events.open_auto_event = open_auto_event;
}
