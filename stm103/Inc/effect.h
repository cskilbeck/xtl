#pragma once

//////////////////////////////////////////////////////////////////////

#pragma pack(push, 1)
struct color_range
{
    color low;
    color high;
    
    color get() const
    {
        int r = rand() % (high.red() - low.red() + 1) + low.red();
        int g = rand() % (high.green() - low.green() + 1) + low.green();
        int b = rand() % (high.blue() - low.blue() + 1) + low.blue();
        return color(r, g, b);
    }
};
#pragma pack(pop)

//////////////////////////////////////////////////////////////////////

#pragma pack(push, 1)
struct speed_range
{
    int16 low;
    int16 high;
    
    int16 get() const
    {
        return rand() % (high - low + 1) + low;
    }
};
#pragma pack(pop)

//////////////////////////////////////////////////////////////////////

struct effect;

struct effect_descriptor
{
    void (*init)(effect *p);
    void (*update)(effect *p);
};

enum { effect_buffer_size = 2048 };

typedef effect_descriptor const effect_desc;

//////////////////////////////////////////////////////////////////////

struct effect
{
    static int const max_color_parameters = 8;
    static int const max_speed_parameters = 8;

    color *rgb_buffer;
    effect_desc *current_desc;
    color_range color_ranges[max_color_parameters];
    speed_range speed_ranges[max_speed_parameters];

    void init();
    void update();
    void cls(color c);

    color get_color_parameter(int index);
    byte get_speed_parameter(int index);

    static void reset();
    static void frame_update();
    static bool setup(message::led_state &e);
    
    void set_pixel(color c, int led);
    void set_pixels(color c, int led, int n, int stride = 1);
};

//////////////////////////////////////////////////////////////////////

void set_effect(int string, int effect_index);
void set_effect_desc(int i, effect_desc *desc);
effect *get_effect_buffer(int i);
effect_desc *get_effect_descriptor(int i);
color gamma_correct(color c);

//////////////////////////////////////////////////////////////////////

