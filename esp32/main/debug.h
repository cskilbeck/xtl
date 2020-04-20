#pragma once

struct debug_led_t
{
    enum effect_type
    {
        off = 0,
        solid = 1,
        blink = 2,
        pulse = 3,
        flash = 4,
    };

    enum color
    {
        blue = 0,
        green = 1,
        cyan = 2,
        red = 3,
        magenta = 4,
        yellow = 5,
        white = 6
    };

    // call this once to setup the hardware
    void init();

    // call this each frame
    void update();

    // set a new debug led effect
    void set(effect_type effect, color c);

    // flash it a certain color for a moment (min 10 ms)
    void set_for(int milliseconds, effect_type effect, color c);
};

struct debug_pin
{
    enum
    {
        off = 0,
        on = 1
    };
};

// don't create your own debug objects, that won't work
extern struct debug_led_t debug_led;
