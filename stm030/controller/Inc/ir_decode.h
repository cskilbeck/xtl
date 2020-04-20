//////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////
// ir remote key codes

enum keys
{
    key_bright_up = 0x90,
    key_bright_down = 0xb8,
    key_off = 0xf8,
    key_on = 0xb0,
    key_red = 0x98,
    key_green = 0xd8,
    key_blue = 0x88,
    key_white = 0xa8,
    key_00 = 0xe8,
    key_01 = 0x48,
    key_02 = 0x68,
    key_flash = 0xb2,
    key_10 = 0x02,
    key_11 = 0x32,
    key_12 = 0x20,
    key_strobe = 0x00,
    key_20 = 0x50,
    key_21 = 0x78,
    key_22 = 0x70,
    key_fade = 0x58,
    key_30 = 0x38,
    key_31 = 0x28,
    key_32 = 0xf0,
    key_smooth = 0x30
};

//////////////////////////////////////////////////////////////////////

extern volatile int ir_key;       // key that was last pressed
extern volatile int ir_press;     // set to 1 when key pressed (clear it in main loop to find out when next key press happens)
extern volatile int ir_repeat;    // how many times the key has been repeated (0 for first press)

//////////////////////////////////////////////////////////////////////
// see main.c for timer setup

void ir_falling_edge(int timer_value);
void ir_reset(void);
