//////////////////////////////////////////////////////////////////////

#include "main.h"
#include "color.h"

//////////////////////////////////////////////////////////////////////

int brightness = 256;
int power = 1;

static int target_brightness = 16;
static int actual_brightness = 16;

//////////////////////////////////////////////////////////////////////
// one gamma table for all channels, which is lame
// a better solution would take the whole rgb into account

byte const gamma_table[] = { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,
                             1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3,   4,   4,
                             4,   4,   4,   5,   5,   5,   5,   6,   6,   6,   6,   7,   7,   7,   7,   8,   8,   8,   9,   9,   9,   10,  10,  10,  11,  11,  11,  12,  12,
                             13,  13,  13,  14,  14,  15,  15,  16,  16,  17,  17,  18,  18,  19,  19,  20,  20,  21,  21,  22,  22,  23,  24,  24,  25,  25,  26,  27,  27,
                             28,  29,  29,  30,  31,  32,  32,  33,  34,  35,  35,  36,  37,  38,  39,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  50,  51,
                             52,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  66,  67,  68,  69,  70,  72,  73,  74,  75,  77,  78,  79,  81,  82,  83,  85,  86,
                             87,  89,  90,  92,  93,  95,  96,  98,  99,  101, 102, 104, 105, 107, 109, 110, 112, 114, 115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133,
                             135, 137, 138, 140, 142, 144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175, 177, 180, 182, 184, 186, 189, 191, 193,
                             196, 198, 200, 203, 205, 208, 210, 213, 215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255 };

//////////////////////////////////////////////////////////////////////
// convert r,g,b into a color

color color_from_rgb(int r, int g, int b)
{
    color c = { (byte)r, (byte)g, (byte)b };
    return c;
};

//////////////////////////////////////////////////////////////////////
// call this each frame

void color_update()
{
    brightness = brightness + (target_brightness - brightness) / 4;
    actual_brightness = power * brightness;
}

//////////////////////////////////////////////////////////////////////
// add something to the target brightness

void change_brightness(int delta)
{
    target_brightness += delta;
    if(target_brightness < 16) {
        target_brightness = 16;
    }
    if(target_brightness > 256) {
        target_brightness = 256;
    }
}

//////////////////////////////////////////////////////////////////////
// correct a color for gamma, power, global brightness

color gamma_correct(color c)
{
    int b = actual_brightness;
    return color_from_rgb((gamma_table[c.r] * b) >> 8, (gamma_table[c.g] * b) >> 8, (gamma_table[c.b] * b) >> 8);
}

//////////////////////////////////////////////////////////////////////
// lerp between two colors, l = 0..256

color color_lerp(color x, color y, int l)
{
    int r = ((y.r - x.r) * l) >> 8;
    int g = ((y.g - x.g) * l) >> 8;
    int b = ((y.b - x.b) * l) >> 8;
    return color_from_rgb(r + x.r, g + x.g, b + x.b);
}
