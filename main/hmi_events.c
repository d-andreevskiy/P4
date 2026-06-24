#include "hmi.h"

// Функции остаются строго локальными (static)
static void settings_event(lv_event_t *e)
{
    LV_UNUSED(e);
    if (auto_screen_active)
        set_message(auto_msg_label, "Открытие настроек пока не реализовано");
    else
        hmi_manual_page.set_message( "Открытие настроек пока не реализовано");
}

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

    if (actual_pressure < 0.0f)
        actual_pressure = 0.0f;
    if (actual_pressure > HMI_PRESSURE_MAX)
        actual_pressure = HMI_PRESSURE_MAX;
        
    update_pressure_ui();
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

// Экспортируем указатели наружу через константную структуру
const hmi_events_t hmi_events = {
    .settings_event = settings_event,
    .pressure_timer_cb = pressure_timer_cb,
    .clock_timer_cb = clock_timer_cb
};
