//////////////////////////////////////////////////////////////////////

#include "main.h"
#include "ir_decode.h"

//////////////////////////////////////////////////////////////////////

volatile int ir_press = 0;
volatile int ir_key = 0;
volatile int ir_repeat = 0;

//////////////////////////////////////////////////////////////////////

enum state_t
{
    init = 0,
    agc = 1,
    get_bits = 2
};

//////////////////////////////////////////////////////////////////////

static enum state_t state = init;
static int bit_count = 0;
static uint32 bits = 0;

//////////////////////////////////////////////////////////////////////
// reset state machine but not keypress

void ir_reset()
{
    state = init;
}

//////////////////////////////////////////////////////////////////////
// simple state machine to decode ir signals
// timer_value is in 100ths of a millisecond

void ir_falling_edge(int timer_value)
{
    switch(state) {

    case init:
        state = agc;
        break;

    case agc:
        if(timer_value < 1100 || timer_value > 1375) {
            state = init;
            ir_repeat = 0;
            ir_press = 0;
            ir_key = 0;
        } else if(timer_value > 1237) {
            bit_count = 0;
            bits = 0;
            state = get_bits;
        } else {
            state = init;
            if(ir_key != 0) {
                ir_repeat += 1;
                ir_press = 1;
            }
        }
        break;

    case get_bits:
        if(timer_value > 250 || timer_value < 100) {
            state = init;
            ir_repeat = 0;
            ir_press = 0;
            ir_key = 0;
        } else {
            bits <<= 1;
            if(timer_value > 168) {
                bits |= 1;
            }
            bit_count += 1;
            if(bit_count >= 32) {
                state = init;
                uint32 key = (bits >> 8) & 0xff;
                uint32 chk = ~bits & 0xff;
                if((key ^ chk) == 0) {
                    ir_key = key;
                    ir_repeat = 0;
                    ir_press = 1;
                }
            }
        }
        break;
    }
}
