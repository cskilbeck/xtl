//////////////////////////////////////////////////////////////////////

#include "types.h"
#include "main.h"
#include "stm32f1xx_hal.h"

//////////////////////////////////////////////////////////////////////

extern "C" void flash(int on, int off)
{
    while(1) {
        set_debug_led(true);
        for(volatile int i = 0; i < on; ++i) {
        }
        set_debug_led(false);
        for(volatile int i = 0; i < off; ++i) {
        }
    }
}

//////////////////////////////////////////////////////////////////////

extern "C" void assert_failed(uint8_t *file, uint32_t line)
{
    flash(5000000, 100000);
}

//////////////////////////////////////////////////////////////////////

extern "C" void _Error_Handler(char *file, int line)
{
    flash(1000000, 500000);
}
