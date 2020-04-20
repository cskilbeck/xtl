//////////////////////////////////////////////////////////////////////

#include "main.h"
#include "spi_leds.h"

//////////////////////////////////////////////////////////////////////

int frames = 0;

//////////////////////////////////////////////////////////////////////

#define SPI_SIZE 30    // # of uint32s in circular SPI buffer

static uint32 frame_buffer[2][NUM_LED_DWORDS];
static uint32 spi_buffer[SPI_SIZE];
static volatile uint32 *source_address;
static volatile uint32 *end_address;
static volatile int drain = 0;

//////////////////////////////////////////////////////////////////////
// table for expanding one byte to 5 bytes
// bits are translated so
// 0 = 10000
// 1 = 11110

// NB these are actually inverted (01111 / 00001) because the level-shifter inverts the signal
// see also use of 0xffffffff for the reset section in fill_half() rather than 0x00000000

uint64 const five_1[256] = {
    0xefbdf7de7b, 0xe1bdf7de7b, 0x2fbcf7de7b, 0x21bcf7de7b, 0xef85f7de7b, 0xe185f7de7b, 0x2f84f7de7b, 0x2184f7de7b, 0xefbdf0de7b, 0xe1bdf0de7b, 0x2fbcf0de7b, 0x21bcf0de7b,
    0xef85f0de7b, 0xe185f0de7b, 0x2f84f0de7b, 0x2184f0de7b, 0xefbd17de7b, 0xe1bd17de7b, 0x2fbc17de7b, 0x21bc17de7b, 0xef8517de7b, 0xe18517de7b, 0x2f8417de7b, 0x218417de7b,
    0xefbd10de7b, 0xe1bd10de7b, 0x2fbc10de7b, 0x21bc10de7b, 0xef8510de7b, 0xe18510de7b, 0x2f8410de7b, 0x218410de7b, 0xefbdf7c27b, 0xe1bdf7c27b, 0x2fbcf7c27b, 0x21bcf7c27b,
    0xef85f7c27b, 0xe185f7c27b, 0x2f84f7c27b, 0x2184f7c27b, 0xefbdf0c27b, 0xe1bdf0c27b, 0x2fbcf0c27b, 0x21bcf0c27b, 0xef85f0c27b, 0xe185f0c27b, 0x2f84f0c27b, 0x2184f0c27b,
    0xefbd17c27b, 0xe1bd17c27b, 0x2fbc17c27b, 0x21bc17c27b, 0xef8517c27b, 0xe18517c27b, 0x2f8417c27b, 0x218417c27b, 0xefbd10c27b, 0xe1bd10c27b, 0x2fbc10c27b, 0x21bc10c27b,
    0xef8510c27b, 0xe18510c27b, 0x2f8410c27b, 0x218410c27b, 0xefbdf75e78, 0xe1bdf75e78, 0x2fbcf75e78, 0x21bcf75e78, 0xef85f75e78, 0xe185f75e78, 0x2f84f75e78, 0x2184f75e78,
    0xefbdf05e78, 0xe1bdf05e78, 0x2fbcf05e78, 0x21bcf05e78, 0xef85f05e78, 0xe185f05e78, 0x2f84f05e78, 0x2184f05e78, 0xefbd175e78, 0xe1bd175e78, 0x2fbc175e78, 0x21bc175e78,
    0xef85175e78, 0xe185175e78, 0x2f84175e78, 0x2184175e78, 0xefbd105e78, 0xe1bd105e78, 0x2fbc105e78, 0x21bc105e78, 0xef85105e78, 0xe185105e78, 0x2f84105e78, 0x2184105e78,
    0xefbdf74278, 0xe1bdf74278, 0x2fbcf74278, 0x21bcf74278, 0xef85f74278, 0xe185f74278, 0x2f84f74278, 0x2184f74278, 0xefbdf04278, 0xe1bdf04278, 0x2fbcf04278, 0x21bcf04278,
    0xef85f04278, 0xe185f04278, 0x2f84f04278, 0x2184f04278, 0xefbd174278, 0xe1bd174278, 0x2fbc174278, 0x21bc174278, 0xef85174278, 0xe185174278, 0x2f84174278, 0x2184174278,
    0xefbd104278, 0xe1bd104278, 0x2fbc104278, 0x21bc104278, 0xef85104278, 0xe185104278, 0x2f84104278, 0x2184104278, 0xefbdf7de0b, 0xe1bdf7de0b, 0x2fbcf7de0b, 0x21bcf7de0b,
    0xef85f7de0b, 0xe185f7de0b, 0x2f84f7de0b, 0x2184f7de0b, 0xefbdf0de0b, 0xe1bdf0de0b, 0x2fbcf0de0b, 0x21bcf0de0b, 0xef85f0de0b, 0xe185f0de0b, 0x2f84f0de0b, 0x2184f0de0b,
    0xefbd17de0b, 0xe1bd17de0b, 0x2fbc17de0b, 0x21bc17de0b, 0xef8517de0b, 0xe18517de0b, 0x2f8417de0b, 0x218417de0b, 0xefbd10de0b, 0xe1bd10de0b, 0x2fbc10de0b, 0x21bc10de0b,
    0xef8510de0b, 0xe18510de0b, 0x2f8410de0b, 0x218410de0b, 0xefbdf7c20b, 0xe1bdf7c20b, 0x2fbcf7c20b, 0x21bcf7c20b, 0xef85f7c20b, 0xe185f7c20b, 0x2f84f7c20b, 0x2184f7c20b,
    0xefbdf0c20b, 0xe1bdf0c20b, 0x2fbcf0c20b, 0x21bcf0c20b, 0xef85f0c20b, 0xe185f0c20b, 0x2f84f0c20b, 0x2184f0c20b, 0xefbd17c20b, 0xe1bd17c20b, 0x2fbc17c20b, 0x21bc17c20b,
    0xef8517c20b, 0xe18517c20b, 0x2f8417c20b, 0x218417c20b, 0xefbd10c20b, 0xe1bd10c20b, 0x2fbc10c20b, 0x21bc10c20b, 0xef8510c20b, 0xe18510c20b, 0x2f8410c20b, 0x218410c20b,
    0xefbdf75e08, 0xe1bdf75e08, 0x2fbcf75e08, 0x21bcf75e08, 0xef85f75e08, 0xe185f75e08, 0x2f84f75e08, 0x2184f75e08, 0xefbdf05e08, 0xe1bdf05e08, 0x2fbcf05e08, 0x21bcf05e08,
    0xef85f05e08, 0xe185f05e08, 0x2f84f05e08, 0x2184f05e08, 0xefbd175e08, 0xe1bd175e08, 0x2fbc175e08, 0x21bc175e08, 0xef85175e08, 0xe185175e08, 0x2f84175e08, 0x2184175e08,
    0xefbd105e08, 0xe1bd105e08, 0x2fbc105e08, 0x21bc105e08, 0xef85105e08, 0xe185105e08, 0x2f84105e08, 0x2184105e08, 0xefbdf74208, 0xe1bdf74208, 0x2fbcf74208, 0x21bcf74208,
    0xef85f74208, 0xe185f74208, 0x2f84f74208, 0x2184f74208, 0xefbdf04208, 0xe1bdf04208, 0x2fbcf04208, 0x21bcf04208, 0xef85f04208, 0xe185f04208, 0x2f84f04208, 0x2184f04208,
    0xefbd174208, 0xe1bd174208, 0x2fbc174208, 0x21bc174208, 0xef85174208, 0xe185174208, 0x2f84174208, 0x2184174208, 0xefbd104208, 0xe1bd104208, 0x2fbc104208, 0x21bc104208,
    0xef85104208, 0xe185104208, 0x2f84104208, 0x2184104208
};

//////////////////////////////////////////////////////////////////////
// stash_5 - convert 1 input dword into 5 output dwords
// heavy register pressure, let the compiler work it out

static void stash_5(uint32_t *dst, uint32_t s)
{
    uint64_t a = five_1[s & 0xff];
    uint64_t b = five_1[(s >> 8) & 0xff];
    uint64_t c = five_1[(s >> 16) & 0xff];
    uint64_t d = five_1[(s >> 24) & 0xff];

    dst[0] = (uint32_t)a;
    dst[1] = (uint32_t)((a >> 32) | (b << 8));
    dst[2] = (uint32_t)((b >> 24) | (c << 16));
    dst[3] = (uint32_t)((c >> 16) | (d << 24));
    dst[4] = (uint32_t)(d >> 8);
}

//////////////////////////////////////////////////////////////////////
// fill either the first or second half of the spi dma buffer

static void fill_half(uint32 *dst)
{
    if(source_address < end_address) {

        for(int i = 0; i < (SPI_SIZE / 10); ++i) {
            stash_5(dst, *source_address++);
            dst += 5;
        }
    } else {
        // declare these volatile to defeat memset optimization which is slower
        volatile uint32 *p = dst;
        volatile uint32 *q = dst + SPI_SIZE / 2;
        while(p < q) {
            *p++ = 0xffffffff;
        }
        drain -= 1;
    }
}

//////////////////////////////////////////////////////////////////////
// spi dma reached halfway, fill in the first half

void HAL_SPI_TxHalfCpltCallback(SPI_HandleTypeDef *spi)
{
    fill_half(spi_buffer);
}

//////////////////////////////////////////////////////////////////////
// spi dma reached the end of the buffer, fill in the second half

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *spi)
{
    fill_half(spi_buffer + (SPI_SIZE / 2));
}

//////////////////////////////////////////////////////////////////////
// wait for previous transmit to complete,
// toggle rgb back buffer
// prime the spi dma buffer
// kick off another

void spi_leds_send()
{
    while(drain > 0) {
    }

    HAL_SPI_DMAStop(&hspi1);
    drain = 3;
    source_address = frame_buffer[frames & 1];
    end_address = source_address + NUM_LED_DWORDS;
    fill_half(spi_buffer);
    fill_half(spi_buffer + (SPI_SIZE / 2));

    HAL_SPI_Transmit_DMA(&hspi1, (uint8_t *)spi_buffer, SPI_SIZE * 4);

    frames += 1;
}

//////////////////////////////////////////////////////////////////////
// get rgb frame buffer to render into this frame

uint32 *current_frame_buffer()
{
    return frame_buffer[frames & 1];
}
