//////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////

#define NUM_LEDS 400
#define NUM_LED_DWORDS ((NUM_LEDS)*3 / 4)

//////////////////////////////////////////////////////////////////////

void spi_leds_send(void);
uint32 *current_frame_buffer(void);

//////////////////////////////////////////////////////////////////////

extern int frames;
