//////////////////////////////////////////////////////////////////////
// neopixels

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <cstddef>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/hw_timer.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_smartconfig.h"
#include "smartconfig_ack.h"

#include "util.h"
#include "debug.h"
#include "neopixel.h"
#include "wifi.h"
#include "button.h"
#include "color.h"
#include "settings.h"
#include "effect.h"

//////////////////////////////////////////////////////////////////////

TaskHandle_t vblank_task_handle = null;
int frames = 0;

//////////////////////////////////////////////////////////////////////

void IRAM_ATTR hw_timer_callback(void *arg)
{
    if(xTaskResumeFromISR(vblank_task_handle)) {
        taskYIELD();
    }
}

//////////////////////////////////////////////////////////////////////

void powerswitch_init()
{
    gpio_config_t c;
    c.pin_bit_mask = GPIO_Pin_5 | GPIO_Pin_4;
    c.mode = GPIO_MODE_OUTPUT;
    c.pull_up_en = GPIO_PULLUP_DISABLE;
    c.pull_down_en = GPIO_PULLDOWN_DISABLE;
    c.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&c);
}

//////////////////////////////////////////////////////////////////////

void powerswitch_set(bool on_or_off)
{
    int x = int(on_or_off);
    gpio_set_level(GPIO_NUM_5, 1 - x);
    gpio_set_level(GPIO_NUM_4, x);
}

//////////////////////////////////////////////////////////////////////

void frame_update()
{
    byte *frame_buffer = neopixel_get_frame_buffer();
    effect::frame_update(frame_buffer);

#if 0
    int x = (frames * 5) & 0xff;
    x = (x * x) >> 8;
    int r = x;
    int g = (x * 0xb0) >> 8;
    int b = (x * 0xb0) >> 8;
    byte *p = frame_buffer;
    for(int i = 0; i < num_leds; ++i) {
        *p++ = r;
        *p++ = g;
        *p++ = b;
    }
#endif
}

//////////////////////////////////////////////////////////////////////

void vblank_task(void *)
{
    hw_timer_init(hw_timer_callback, null);
    hw_timer_alarm_us(1000000 / 60, true);

    neopixel_init(num_leds);

    debug_set_color(debug_color::green);
    button_init();

    while(1) {
        vTaskSuspend(null);
        neopixel_update();
        button_update();
        if(button_pressed) {
            debug_flash(debug_color::red, debug_color::off, 5, 5);
        }
        debug_update();
        frame_update();
        frames += 1;
    }
}

//////////////////////////////////////////////////////////////////////

extern "C" void app_main()
{
    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    settings_queue = xQueueCreate(2, sizeof(settings_t));
    settings_t::init();
    powerswitch_init();
    powerswitch_set(true);

    debug_init();
    initialise_wifi();
    xTaskCreate(vblank_task, "vblank_task", 2048, null, 15, &vblank_task_handle);
}
