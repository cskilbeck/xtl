//////////////////////////////////////////////////////////////////////
// functions for driving 3 PWM channels to the RGB debug led

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "util.h"
#include "debug.h"

//////////////////////////////////////////////////////////////////////

debug_led_t debug_led;

//////////////////////////////////////////////////////////////////////

namespace
{
    //////////////////////////////////////////////////////////////////////
    // 13 bits per channel (0..8191)

    struct color_t
    {
        int r, g, b;
    };

    //////////////////////////////////////////////////////////////////////

    struct effector
    {
        virtual int get(int frame) = 0;
    };

    //////////////////////////////////////////////////////////////////////

    color_t *led_color;
    effector *cur_effect;

    color_t *flash_color;
    effector *flash_effect;

    TickType_t flash_end;

    //////////////////////////////////////////////////////////////////////

    color_t colors[] = {
        { 0, 0, 8191 },         // blue
        { 0, 8191, 0 },         // green
        { 0, 8191, 8191 },      // cyan
        { 8191, 0, 0 },         // red
        { 8191, 0, 8191 },      // magenta
        { 8191, 5000, 0 },      // yellow
        { 8191, 7200, 6700 }    // white
    };

    int frames = 0;

    //////////////////////////////////////////////////////////////////////

    struct pulse_led_effect_t : effector
    {
        int get(int frame) override
        {
            return 8191 - abs(((frame << 8) & 0x3fff) - 8191);
        }
    } pulse_led_effect;

    //////////////////////////////////////////////////////////////////////

    struct blink_led_effect_t : effector
    {
        int get(int frame) override
        {
            return ((frame >> 4) & 1) * 8191;
        }
    } blink_led_effect;

    //////////////////////////////////////////////////////////////////////

    struct flash_led_effect_t : effector
    {
        int get(int frame) override
        {
            return ((frame >> 2) & 1) * 8191;
        }
    } flash_led_effect;

    //////////////////////////////////////////////////////////////////////

    struct solid_led_effect_t : effector
    {
        int get(int frame) override
        {
            return 8191;
        }
    } solid_led_effect;

    //////////////////////////////////////////////////////////////////////

    struct off_led_effect_t : effector
    {
        int get(int frame) override
        {
            return 0;
        }
    } off_led_effect;

    //////////////////////////////////////////////////////////////////////
    // keep these lined up with the enum effect

    effector *led_effects[] = { &off_led_effect, &solid_led_effect, &blink_led_effect, &pulse_led_effect, &flash_led_effect };

    //////////////////////////////////////////////////////////////////////
    // pwm admin

    constexpr auto LEDC_TIMER_R = LEDC_TIMER_0;
    constexpr auto LEDC_TIMER_G = LEDC_TIMER_2;
    constexpr auto LEDC_TIMER_B = LEDC_TIMER_1;
    constexpr auto LEDC_HS_CHR_GPIO = 17;    // IF THE PCB YE MAKE MORE NICE
    constexpr auto LEDC_HS_CHG_GPIO = 05;    // CHECK THESE NUMBERS SHALL THEE, THRICE
    constexpr auto LEDC_HS_CHB_GPIO = 16;
    constexpr auto LEDC_HS_CHR_CHANNEL = LEDC_CHANNEL_0;
    constexpr auto LEDC_HS_CHG_CHANNEL = LEDC_CHANNEL_2;
    constexpr auto LEDC_HS_CHB_CHANNEL = LEDC_CHANNEL_1;
    constexpr auto GPIO_G = GPIO_NUM_21;
    constexpr auto GPIO_R = GPIO_NUM_18;
    constexpr auto GPIO_B = GPIO_NUM_19;

    ledc_channel_config_t ledc_channel[3] = { LEDC_HS_CHR_GPIO, LEDC_HIGH_SPEED_MODE, LEDC_HS_CHR_CHANNEL, LEDC_INTR_DISABLE, LEDC_TIMER_R, 8191, 0,
                                              LEDC_HS_CHG_GPIO, LEDC_HIGH_SPEED_MODE, LEDC_HS_CHG_CHANNEL, LEDC_INTR_DISABLE, LEDC_TIMER_G, 8191, 0,
                                              LEDC_HS_CHB_GPIO, LEDC_HIGH_SPEED_MODE, LEDC_HS_CHB_CHANNEL, LEDC_INTR_DISABLE, LEDC_TIMER_B, 8191, 0 };

}    // namespace

//////////////////////////////////////////////////////////////////////

void debug_led_t::init()
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE, .duty_resolution = LEDC_TIMER_13_BIT, .timer_num = LEDC_TIMER_0, .freq_hz = 5000, .clk_cfg = LEDC_AUTO_CLK
    };

    for(int i = 0; i < 3; ++i) {
        ledc_timer.timer_num = ledc_channel[i].timer_sel;
        ledc_timer_config(&ledc_timer);
        ledc_channel_config(&ledc_channel[i]);
    }
    flash_color = null;
    set(solid, red);
}

//////////////////////////////////////////////////////////////////////
// call this at 10Hz or more

void debug_led_t::update()
{
    if(cur_effect == null) {
        return;
    }

    color_t *c;
    effector *e;

    if(flash_effect != null && xTaskGetTickCount() < flash_end) {
        e = flash_effect;
        c = flash_color;
    } else {
        flash_effect = null;
        e = cur_effect;
        c = led_color;
    }

    int v = e->get(frames);

    // make it the right color
    int r = (c->r * v) >> 13;
    int g = (c->g * v) >> 13;
    int b = (c->b * v) >> 13;

    // cube it for a ghetto gamma ramp
    // invert because common anode
    r = 8191 - ((((r * r) >> 13) * r) >> 13);
    g = 8191 - ((((g * g) >> 13) * g) >> 13);
    b = 8191 - ((((b * b) >> 13) * b) >> 13);

    // set the pwm rgb channels
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_HS_CHR_CHANNEL, r);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_HS_CHG_CHANNEL, g);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_HS_CHB_CHANNEL, b);

    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_HS_CHR_CHANNEL);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_HS_CHG_CHANNEL);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_HS_CHB_CHANNEL);

    frames += 1;
}

//////////////////////////////////////////////////////////////////////

void debug_led_t::set_for(int milliseconds, debug_led_t::effect_type effect, debug_led_t::color color)
{
    frames = 0;
    flash_color = colors + (int)(color % countof(colors));
    flash_effect = led_effects[effect % countof(led_effects)];
    flash_end = xTaskGetTickCount() + milliseconds / portTICK_PERIOD_MS;
}

//////////////////////////////////////////////////////////////////////

void debug_led_t::set(debug_led_t::effect_type effect, debug_led_t::color c)
{
    frames = 0;
    led_color = colors + (int)(c % countof(colors));
    cur_effect = led_effects[effect % countof(led_effects)];
}
