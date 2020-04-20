//////////////////////////////////////////////////////////////////////
// TODO (chs): build firmware flasher

// DONE (chs): save/load effect to/from nvram
// DONE (chs): effect player on RGB leds
// DONE (chs): RGB driver in a task / timer

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp32/rom/gpio.h"
#include "soc/timer_group_struct.h"
#include "driver/timer.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "util.h"
#include "neopixel.h"
#include "color.h"
#include "settings.h"
#include "effect.h"
#include "debug.h"
#include "wifi.h"

static char const *TAG = "Main";

static byte *frame[2];

//////////////////////////////////////////////////////////////////////

static intr_handle_t s_timer_handle;
static volatile TaskHandle_t vblank_task_handle;

static constexpr uint32 VBLANK_BIT = 1;

static int cur_frame = 0;
static int frames = 0;

//////////////////////////////////////////////////////////////////////

static void IRAM_ATTR timer_isr(void *arg)
{
    TIMERG0.int_clr_timers.t0 = 1;
    TIMERG0.hw_timer[0].config.alarm_en = 1;

    BaseType_t woke;
    if(xTaskNotifyFromISR(vblank_task_handle, 0, eNoAction, &woke) == pdPASS && woke) {
        portYIELD_FROM_ISR();
    }
}

//////////////////////////////////////////////////////////////////////

void vblank_task(void *)
{
    settings_queue = xQueueCreate(2, sizeof(settings_t));

    settings_t::init();

    neopixel_init();    // Call this from within this task otherwise they flicker!? Either a core thing or a priority thing

    vblank_task_handle = xTaskGetCurrentTaskHandle();

    ESP_LOGI(TAG, "vblank_task_handle = %p", vblank_task_handle);

    // CRASHES WITHOUT THIS DELAY!?
    vTaskDelay(10);

    timer_config_t config = {
        .alarm_en = TIMER_ALARM_EN,
        .counter_en = TIMER_PAUSE,
        .intr_type = TIMER_INTR_LEVEL,
        .counter_dir = TIMER_COUNT_UP,
        .auto_reload = TIMER_AUTORELOAD_EN,
        .divider = 80    // 1 us per tick
    };

    timer_init(TIMER_GROUP_0, TIMER_0, &config);
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, 20000);
    timer_enable_intr(TIMER_GROUP_0, TIMER_0);
    timer_isr_register(TIMER_GROUP_0, TIMER_0, &timer_isr, null, ESP_INTR_FLAG_IRAM, &s_timer_handle);
    timer_start(TIMER_GROUP_0, TIMER_0);

    while(true) {

        xTaskNotifyWait(0, ULONG_MAX, null, portMAX_DELAY);

        if(frames > 10) {
            neopixel_write(frame[cur_frame], num_leds);    // wait for previous dma  and kick off the new one
            cur_frame = 1 - cur_frame;
            effect::frame_update(frame[cur_frame]);    // set pixels in current frame buffer
        }
        debug_led.update();
        frames += 1;
    }
}

//////////////////////////////////////////////////////////////////////

void init_vblank_task()
{
    xTaskCreatePinnedToCore(vblank_task, "vblank_task", 4096, null, 1, null, 1 - wifi_core);    // this sends the frame to the leds, don't share with wifi core!
}

//////////////////////////////////////////////////////////////////////

void init_nvs()
{
    debug_led.set(debug_led.solid, debug_led.blue);
    esp_err_t err = nvs_flash_init();
    if(err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

//////////////////////////////////////////////////////////////////////

void do_main()
{
    vTaskDelay(100);
    frame[0] = new byte[num_leds * 3];
    frame[1] = new byte[num_leds * 3];
    ESP_LOGI(TAG, "DO DO DO do_main: ESP-IDF VERSION " IDF_VER);
    debug_led.init();
    init_nvs();
    debug_led.set(debug_led.pulse, debug_led.magenta);
    init_vblank_task();    // start 50Hz timer interrupt
    initialise_wifi();

    // main task just idle (should we delete it?)
    while(true) {
        vTaskDelay(portMAX_DELAY);
    }
}

//////////////////////////////////////////////////////////////////////

extern "C" {
void app_main()
{
    do_main();
}
}
