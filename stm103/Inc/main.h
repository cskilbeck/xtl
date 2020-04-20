#pragma once

//////////////////////////////////////////////////////////////////////

#ifdef USE_FULL_ASSERT
#define assert(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
#else
#define assert(expr) ((void)0U)
#endif

//////////////////////////////////////////////////////////////////////

extern "C" void assert_failed(uint8 *file, uint32 line);
extern "C" void _Error_Handler(char *, int);
extern "C" void flash(int, int);

#define Error_Handler() _Error_Handler(__FILE__, __LINE__)

//////////////////////////////////////////////////////////////////////

static uint32 const max_leds = 50;                                   // max # of leds on the longest string
static uint32 const max_strings = 6;                                 // max # of strings of leds, must be 8 or 16
static uint32 const max_total_leds = max_leds * max_strings;         // = 1600 leds total possible
static uint32 const bytes_per_string = max_leds * 3;                 // bytes per led string
static uint32 const buffer_size = bytes_per_string * max_strings;    // total RGB24 bytes

extern byte frame_buffer[];											 // draw your colours into here

struct color;

extern color * const color_buffer;

extern volatile uint32 frames;

extern void set_debug_led(bool on);
extern void set_pulse_led(int n);   // 0 = off, 4095 = brightest

//////////////////////////////////////////////////////////////////////

#define BITBAND_BASE 0x22000000
#define BITBAND_ADDR(addr, bit) ((volatile uint32 *)(BITBAND_BASE + (((uint32)addr) - SRAM_BASE) * 32 + (bit)*4))
    
#define SET_GPIO(gpio, pin) gpio->BSRR = pin
#define CLR_GPIO(gpio, pin) gpio->BSRR = (pin << 16)

