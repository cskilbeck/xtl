//////////////////////////////////////////////////////////////////////
// very basic neopixel driver, cannot coexist with other RMT users

#include <cstdint>
#include <algorithm>
#include <stdio.h>
#include "esp_log.h"
#include "driver/rmt.h"
#include "soc/rtc_wdt.h"
#include "util.h"
#include "neopixel.h"

//////////////////////////////////////////////////////////////////////

namespace
{
    char const *TAG = "neopixel";

    constexpr auto LED_RMT_TX_CHANNEL = RMT_CHANNEL_0;
    constexpr auto LED_RMT_TX_GPIO = GPIO_NUM_12;

    constexpr auto T0H = 14;    // 0.35uS
    constexpr auto T0L = 52;    // 1.36uS
    constexpr auto T1H = 52;
    constexpr auto T1L = 14;

    //////////////////////////////////////////////////////////////////////
    // RMT want data

    void IRAM_ATTR rmt_translate(const void *src, rmt_item32_t *dest, size_t src_size, size_t wanted_num, size_t *translated_size, size_t *item_num)
    {
        size_t size = 0;
        size_t num = 0;

        constexpr rmt_item32_t const on = { { { T1H, 0, T1L, 1 } } };    // these are inverted because the level shifter inverts them back
        constexpr rmt_item32_t const off = { { { T0H, 0, T0L, 1 } } };
        constexpr rmt_item32_t const on_off[2] = { off, on };

        if(src != null && dest != null) {
            byte const *psrc = (byte const *)src;
            while(num < wanted_num) {
                byte const data = *psrc++;
                for(int bit = 7; bit >= 0; bit -= 1) {
                    *dest++ = on_off[(data >> bit) & 1];
                }
                num += 8;
                if(++size >= src_size) {
                    dest[-1].duration1 = 2000;    // reset ~50uS
                    break;
                }
            }
        }
        *translated_size = size;
        *item_num = num;
    }

}    // namespace

//////////////////////////////////////////////////////////////////////

void neopixel_init()
{
    rmt_config_t config;
    config.rmt_mode = RMT_MODE_TX;
    config.channel = LED_RMT_TX_CHANNEL;
    config.gpio_num = LED_RMT_TX_GPIO;
    config.mem_block_num = 1;
    config.tx_config.loop_en = false;
    config.tx_config.carrier_en = false;
    config.tx_config.idle_output_en = true;
    config.tx_config.idle_level = RMT_IDLE_LEVEL_HIGH;    // high because level shifter inverts it
    config.clk_div = 2;

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, ESP_INTR_FLAG_IRAM));
    ESP_ERROR_CHECK(rmt_translator_init(config.channel, rmt_translate));
}

//////////////////////////////////////////////////////////////////////

void neopixel_write(byte const *rgb, int num_leds)
{
    ESP_ERROR_CHECK(rmt_wait_tx_done(LED_RMT_TX_CHANNEL, portMAX_DELAY));
    ESP_ERROR_CHECK(rmt_write_sample(LED_RMT_TX_CHANNEL, rgb, num_leds * 3, false));
}
