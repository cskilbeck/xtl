// go 6

// twinkle
// flash
// pulse

#include "types.h"
#include "main.h"
#include "util.h"
#include "printf.h"
#include "message.h"
#include "effect.h"

// buffer for 1 effect per string, effects can be up to 2048 bytes big
__align(8) byte effect_buffer[max_strings * effect_buffer_size];

static int frame = 0;

color global_dimmer(255,220,220);

bool power = true;
byte global_speed = 128;
byte brightness = 255;
int new_state_received = 0;

void set_globals(message::global_state &state)
{
    brightness = state.brightness;
    color black = 0;
    color white(255,220,235);
    global_dimmer = lerp(black, white, (int)brightness * 255 / 100);
    if(!(state.flags & message::global_power)) {
        global_dimmer = 0;
    }
    global_speed = state.speed;
    if(state.flags & message::global_new_state) {
        new_state_received = 5;
        state.flags &= ~message::global_new_state;
    }
}

//////////////////////////////////////////////////////////////////////
// gamma correct and invert for dma

byte const gammaTable[] = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254,
                            254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 253, 253, 253, 253, 253, 253, 253, 253, 252, 252, 252, 252, 252, 252, 252, 251, 251,
                            251, 251, 251, 250, 250, 250, 250, 249, 249, 249, 249, 248, 248, 248, 248, 247, 247, 247, 246, 246, 246, 245, 245, 245, 244, 244, 244, 243, 243,
                            242, 242, 242, 241, 241, 240, 240, 239, 239, 238, 238, 237, 237, 236, 236, 235, 235, 234, 234, 233, 233, 232, 231, 231, 230, 230, 229, 228, 228,
                            227, 226, 226, 225, 224, 223, 223, 222, 221, 220, 220, 219, 218, 217, 216, 216, 215, 214, 213, 212, 211, 210, 209, 208, 207, 206, 205, 205, 204,
                            203, 201, 200, 199, 198, 197, 196, 195, 194, 193, 192, 191, 189, 188, 187, 186, 185, 183, 182, 181, 180, 178, 177, 176, 174, 173, 172, 170, 169,
                            168, 166, 165, 163, 162, 160, 159, 157, 156, 154, 153, 151, 150, 148, 146, 145, 143, 141, 140, 138, 136, 135, 133, 131, 129, 128, 126, 124, 122,
                            120, 118, 117, 115, 113, 111, 109, 107, 105, 103, 101, 99,  97,  95,  93,  91,  88,  86,  84,  82,  80,  78,  75,  73,  71,  69,  66,  64,  62,
                            59,  57,  55,  52,  50,  47,  45,  42,  40,  37,  35,  32,  30,  27,  24,  22,  19,  16,  14,  11,  8,   6,   3,   0 };

//////////////////////////////////////////////////////////////////////

color gamma_correct(color c)
{
    return color(gammaTable[(c.r * global_dimmer.r) >> 8], gammaTable[(c.g * global_dimmer.g) >> 8], gammaTable[(c.b * global_dimmer.b) >> 8]);

}

//////////////////////////////////////////////////////////////////////

struct value
{
    byte val;
    int8 vel;
    color c[2];
    
    enum {
        in_progress = 0,
        hit_top = 1,
        hit_bottom = 2
    };

    void init(color color1, color color2)
    {
        val = 0;
        vel = 16;
        c[0] = color1;
        c[1] = color2;
    }

    void randomize()
    {
        val = 0;
        vel = 0;
        c[0].randomize();
        c[1].randomize();
    }
    
    static int adjust_speed(int s)
    {
        return ((s * global_speed) >> 16) + 2;
    }
    
    static int speed(speed_range const &range, int turbo = 32)
    {
        return adjust_speed(range.get() * turbo);;
    }

    static int fixed_speed(speed_range const &range, int turbo = 32)
    {
        return adjust_speed(((range.low + range.high) / 2) * turbo);
    }

    int update(speed_range const &range, int turbo = 32)
    {
        int v = val;
        int r = in_progress;
        v += vel;
        if(v <= 0) {
            v = 0;
            vel = speed(range, turbo);
            r = hit_bottom;
        } else if(v >= 255) {
            v = 255;
            vel = -speed(range, turbo);
            r = hit_top;
        }
        val = v;
        return r;
    }
    
    int update_fixed(speed_range const &range)
    {
        int v = val;
        int r = in_progress;
        v += vel;
        if(v <= 0) {
            v = 0;
            vel = fixed_speed(range);
            r = hit_bottom;
        } else if(v >= 255) {
            v = 255;
            vel = -fixed_speed(range);
            r = hit_top;
        }
        val = v;
        return r;
    }
    
    int update_rand(color_range const &color_range, speed_range const &speed_range)
    {
        int r = update(speed_range);
        if(r & 1) {
            c[0] = color_range.get();
        } else if(r & 2) {
            c[1] = color_range.get();
        }
        return r;
    }

    int update_random(speed_range const &range)
    {
        int r = update(range);
        if(r == 1) {
            c[0].randomize();
        } else if(r == 2) {
            c[1].randomize();
        }
        return r;
    }

    color color()
    {
        return lerp(c[0], c[1], val);
    }
};

//////////////////////////////////////////////////////////////////////

color effect::get_color_parameter(int index)
{
    return color_ranges[index].get();
}

//////////////////////////////////////////////////////////////////////

byte effect::get_speed_parameter(int index)
{
    return speed_ranges[index].get();
}

//////////////////////////////////////////////////////////////////////

void effect::set_pixel(color c, int led)
{
    rgb_buffer[led] = gamma_correct(c);
}

//////////////////////////////////////////////////////////////////////

void effect::set_pixels(color c, int led, int n, int stride)
{
    if(n == 0) {
        return;
    }
    color t = gamma_correct(c);
    int r = t.r;
    int g = t.g;
    int b = t.b;
    byte *q = (byte *)(rgb_buffer + led);
    stride *= 3;
    do {
        q[0] = r;
        q[1] = g;
        q[2] = b;
        q += stride;
    } while(--n != 0);
}

//////////////////////////////////////////////////////////////////////

void effect::init()
{
}

//////////////////////////////////////////////////////////////////////

void effect::update()
{
}

//////////////////////////////////////////////////////////////////////

void effect::cls(color c)
{
    set_pixels(c, 0, max_leds);
}

//////////////////////////////////////////////////////////////////////

struct red_effect : effect
{
    void init()
    {
        cls(0xff0000);
    }
};

//////////////////////////////////////////////////////////////////////

struct green_effect : effect
{
    void init()
    {
        cls(0x00ff00);
    }
};

//////////////////////////////////////////////////////////////////////

struct blue_effect : effect
{
    void init()
    {
        cls(0x0000ff);
    }
};

//////////////////////////////////////////////////////////////////////

struct yellow_effect : effect
{
    void init()
    {
        cls(0xfff800);
    }
};

//////////////////////////////////////////////////////////////////////

struct cyan_effect : effect
{
    void init()
    {
        cls(0x00ffff);
    }
};

//////////////////////////////////////////////////////////////////////

struct magenta_effect : effect
{
    void init()
    {
        cls(0xff00ff);
    }
};

//////////////////////////////////////////////////////////////////////

struct orange_effect : effect
{
    void init()
    {
        cls(0xff6800);
    }
};

//////////////////////////////////////////////////////////////////////

struct dim_effect : effect
{
    uint32 val;
    void init()
    {
        val = 0;
    }
    
    void update()
    {
        val += 68;
        uint32 c = (val / 16) & 0xff;
        c = c | (c<<8) | (c<<16);
        cls(c);
    }
};

//////////////////////////////////////////////////////////////////////

struct white_effect : effect
{
    void init()
    {
        cls(0xffffff);
    }
};

//////////////////////////////////////////////////////////////////////

struct candy_effect : effect
{
    value w[max_leds];

    void init()
    {
        for(int i=0; i<max_leds; ++i) {
            value &v = w[i];
            v.randomize();
        }
    }

    void update()
    {
        int led = 0;
        color_range r;
        r.low = color(0,0,0);
        r.high = color(255,255,255);
        for(int i=0; i<max_leds; ++i) {
            value &v = w[i];
            v.update_rand(r, speed_ranges[0]);
            set_pixel(v.color(), led++);
        }
    }
};

//////////////////////////////////////////////////////////////////////

struct sine_effect : effect
{
    value c[4];
    value v[2];

    void init()
    {
        for(int i = 0; i < 4; ++i) {
            c[i].init(color::random(), color::random());
            c[i].vel = 20;
        }
        v[0].init(0xff0000, 0x00ff00);
        v[1].init(0x0000ff, 0xffff00);
        v[0].vel = 4;
        v[1].vel = 4;
    }

    void update()
    {
        for(int i = 0; i < 4; ++i) {
            c[i].update_random(speed_ranges[0]);
        }
        v[0].c[0] = c[0].color();
        v[0].c[1] = c[1].color();
        v[1].c[0] = c[2].color();
        v[1].c[1] = c[3].color();

        v[0].update(speed_ranges[1]);
        v[1].update(speed_ranges[2]);

        int base = frame * 16;    // later, sin(frame)
        int t = 32;
        for(int i = 0; i < max_leds; ++i) {
            set_pixel(lerp(v[0].color(), v[1].color(), (sin(base) >> 1) + 128), i);
            base += t;
        }
    }
};

//////////////////////////////////////////////////////////////////////
// pulse between two colours

struct spark_effect : effect
{
    int n;

    void init()
    {
        n = 0;
        cls(color_ranges[0].low);
    }

    void update()
    {
        set_pixel(color_ranges[0].low, n);
        n = rand() % max_leds;
        set_pixel(color_ranges[1].get(), n);
    }
};

//////////////////////////////////////////////////////////////////////

struct pulse_effect : effect
{
    color c[6];

    void init()
    {
        for(int i=0; i<6; ++i) {
            c[i] = get_color_parameter(i);
        }
    }

    void update()
    {
        int f = (global_speed * frames) >> 5;
        int x = (f / 1024) % 6;
        int y = (x + 1) % 6;
        int z = (f & 1023);
        for(int i=0; i<max_leds; ++i) {
            int l = (i * 3 + z) - 512;
            if(l < 0) {
                l = 0;
            }
            if(l > 255) {
                l = 255;
            }
            set_pixel(lerp(c[x], c[y], l), i);
        }
    }
};

//////////////////////////////////////////////////////////////////////

struct solid_effect : effect
{
    color c[max_leds];

    void init()
    {
        for(int i=0; i<max_leds; ++i) {
            c[i] = get_color_parameter(i % 6);
        }
    }

    void update()
    {
        for(int i=0; i<max_leds; ++i) {
            set_pixel(c[i], i);
        }
    }
};

//////////////////////////////////////////////////////////////////////

struct sparkle_effect : effect
{
    value w[max_leds];
    byte id[max_leds];
    color c[6];

    void init()
    {
        Random r(frames);
        for(int i=0; i<6; ++i) {
            c[i] = get_color_parameter(i);
        }
        for(int i=0; i<max_leds; ++i) {
            id[i] = rand() % 6;
            int x = i % 6;
            int y = (x + 1) % 6;
            w[i].init(c[x], c[y]);
            w[i].vel = value::speed(speed_ranges[0], 64) * ((r.next_byte() & 2) - 1);
            w[i].val = r.next_byte();
        }
    }
    
    // every N frames, 
    void update()
    {
        Random r(frames);
        int x;
        for(int i=0; i<max_leds; ++i) {
            switch(w[i].update(speed_ranges[0], 64)) {
                case 0:
                    break;
                case value::hit_bottom:
                    x = id[i] % 6;
                    w[i].c[1] = c[x];
                    c[x] = get_color_parameter(x);  // in case the color is random
                    break;
                case value::hit_top:
                    id[i] += 1;
                    w[i].c[0] = c[(id[i] + 1) % 6];
                    break;
            }
            set_pixel(w[i].color(), i);
        }
    }
};

//////////////////////////////////////////////////////////////////////

struct flash_effect : effect
{
    int seed;
    int speed;
    color c[6];


    // 48 to 104
    
    void init()
    {
        speed = 120 - ((global_speed * 109) >> 8);
        seed = frames;
        for(int i=0; i<6; ++i) {
            c[i] = get_color_parameter(i);
        }
        frame();
    }
    
    // every N frames, 
    void update()
    {
        if((frames % speed) == 0) {
            init();
        }
        else {
            frame();
        }
    }
    
    void frame() {
        Random rnd(seed);
        for(int i=0; i<max_leds; ++i) {
            set_pixel(c[rnd.next() % 6], i);
        }
    }
};

//////////////////////////////////////////////////////////////////////

struct twinkle_effect : effect
{
    value w[max_leds];

    void init()
    {
        Random r(frames);
        color c[6];
        for(int i=0; i<6; ++i) {
            c[i] = get_color_parameter(i);
        }
        for(int i=0; i<max_leds; ++i) {
            int x = r.next() % 6;
            w[i].init(c[x], c[(x + 1) % 6]);
            w[i].vel = value::speed(speed_ranges[0]);
            w[i].val = r.next() & 255;
        }
    }
    
    // every N frames, 
    void update()
    {
        Random r(frames);
        color c[6];
        for(int i=0; i<6; ++i) {
            c[i] = get_color_parameter(i);
        }
        for(int i=0; i<max_leds; ++i) {
            switch(w[i].update(speed_ranges[0])) {
                case 0:
                    break;
                case value::hit_bottom:
                    break;
                case value::hit_top:
                    int x = r.next() % 6;
                    w[i].init(c[x], c[(x + 1) % 6]);
                    w[i].val = 0;
                    w[i].vel = value::speed(speed_ranges[0]);
                    break;
            }
            set_pixel(w[i].color(), i);
        }
    }
};

//////////////////////////////////////////////////////////////////////

struct glow_effect : effect
{
    color c[6];
    int x;
    
    void init()
    {
        x = 0;
        for(int i=0; i<6; ++i) {
            c[i] = get_color_parameter(i);
        }
    }
    
    // every N frames, 
    void update()
    {
        x += global_speed >> 3;
        if(x >= (2048 * 3)) {
            init();
        }
        uint a = (x + 512) / 1024;
        uint p = ((a + 1) & 0xfe) % 6;
        uint q = ((a & 0xfe) + 1) % 6;
        int b = ease((sin(x) + 255) >> 1);
        color r = lerp(c[p], c[q], b);
        for(int i=0; i<max_leds; ++i) {
            set_pixel(r, i);
        }
    }
};

//////////////////////////////////////////////////////////////////////

#define EFFECTOR(x)                   \
    void x##_effect_init(effect *e)   \
    {                                 \
        ((x##_effect *)e)->init();    \
    }                                 \
    void x##_effect_update(effect *e) \
    {                                 \
        ((x##_effect *)e)->update();  \
    }                                 \
    effect_desc x##_effect_creator = { x##_effect_init, x##_effect_update };

#include "effect_list.h"

#undef EFFECTOR
#define EFFECTOR(x) &x##_effect_creator,

effect_desc *descriptors[] = {
#include "effect_list.h"
};

//////////////////////////////////////////////////////////////////////

effect *get_effect_buffer(int i)
{
    return (effect *)(effect_buffer + i * effect_buffer_size);
}

//////////////////////////////////////////////////////////////////////

void set_effect_desc(int i, effect_desc *desc)
{
    get_effect_buffer(i)->current_desc = desc;
}

//////////////////////////////////////////////////////////////////////

effect_desc *get_effect_descriptor(int i)
{
    return descriptors[i];
}

//////////////////////////////////////////////////////////////////////

void effect::reset()
{
    for(int i = 0; i < max_strings; ++i) {
        effect *eff = get_effect_buffer(i);
        eff->rgb_buffer = (color *)(::frame_buffer + i * bytes_per_string);
        eff->current_desc = null;
        memset((byte *)eff->color_ranges, 0, sizeof(color_range) * max_color_parameters);
        memset((byte *)eff->speed_ranges, 0, sizeof(speed_range) * max_speed_parameters);
    }
}

//////////////////////////////////////////////////////////////////////

void effect::frame_update()
{
    frame += 1;
    for(int i = 0; i < max_strings; ++i) {
        effect *eff = get_effect_buffer(i);
        effect_desc *p = eff->current_desc;
        if(p != null) {
            p->update(eff);
        }
    }
    //set_debug_led(new_state_received != 0);
    if(new_state_received > 0) {
        new_state_received -= 1;
        uint32 color = gamma_correct(0);    // off
        for(int i=0; i<max_total_leds; ++i) {
            color_buffer[i] = color;
        }
    }
}

//////////////////////////////////////////////////////////////////////

bool effect::setup(message::led_state &e)
{
    effect_desc *desc = descriptors[e.effect_index];
    effect *eff = get_effect_buffer(e.string_index);
    effect_desc *d = eff->current_desc;
    eff->current_desc = desc;
    memcpy((byte *)eff->color_ranges, (byte *)e.color_ranges, sizeof(color_range) * max_color_parameters);
    memcpy((byte *)eff->speed_ranges, (byte *)e.speed_ranges, sizeof(speed_range) * max_speed_parameters);
    if(d != desc || new_state_received) {
        desc->init(eff);
    }
    return true;
}

//////////////////////////////////////////////////////////////////////

void set_effect(int string, int effect_index)
{
    effect_desc *desc = descriptors[effect_index];
    effect *eff = get_effect_buffer(string);
    eff->current_desc = desc;
    desc->init(eff);
}
