//////////////////////////////////////////////////////////////////////

#include "types.h"
#include "main.h"
#include "util.h"
#include "printf.h"
#include "stm32f1xx_hal.h"
#include "message.h"
#include "effect.h"
#include "spi.h"

// UART3    - debug channel (no DMA)

// SPI2     - ESP8266 channel

// TIMER1   - UPDATE for LED reset
// TIMER2   - UPDATE for 60Hz frame timer
// TIMER3   - UPDATE, CC1, CC3 for LED driver
// TIMER4   - PWM DEBUG LED
// TIMER5   - SPI Request pulse timing

// ADC1     - CH 6 for thermistor

// GPIOs
// A0-A5    - LEDs
// A6       - ADC1 CH 6 (temp sensor)
// B4       - REQUEST data from ESP8266 (active low)
// B5-B6    - DEBUG LEDs
// B10-B11  - UART3
// B12-B15  - SPI2

// DMA1_Channels 3,6,2 for LED driver
// DMA1_Channels 4,5 for SPI2

//////////////////////////////////////////////////////////////////////

ADC_HandleTypeDef hadc1;

UART_HandleTypeDef uart3;
volatile int uart_busy = 0;

SPI_HandleTypeDef hspi2;
DMA_HandleTypeDef hdma_spi2_rx;
DMA_HandleTypeDef hdma_spi2_tx;

TIM_HandleTypeDef htim4;

uint32 spi_errors = 0;
volatile bool spi_data = false;
uint32 spi_pulse_delay = 0;

byte uart3_transmit_buffer[256];
byte uart3_receive_buffer[256];

byte command_buffer[256];
bool command_flag = false;      // if this is true, it hasn't been handled yet

bool adc_flag = false;
int adc_value;
int adc_loop = 0;
int adc_delay = 0;
int adc_readings = 0;
int adc_current_value = 0;

// Timer 3 Period = 89 = ~800KHz, 179 = ~400KHz
uint32 const timer3_period = 89;
// uint32 const timer3_period = 179;

uint32 const off_1_period = timer3_period * 335 / 1000;
uint32 const off_2_period = timer3_period * 665 / 1000;

//////////////////////////////////////////////////////////////////////

volatile uint32 frames = 0;
volatile uint32 can_kick = 1;
volatile uint32 buffer_free = 1;

uint32 const transfer_buffer_size = 2;                                       // number of pixels in the dma buffer - must be > 1 and max_leds must be a multiple of this
uint32 const half_transfer_buffer_size = transfer_buffer_size / 2;           // 1/2 number of pixels in the dma buffer
uint32 const transfer_count = 24 * transfer_buffer_size;                     // dma transfer count per whole dma
uint32 const half_transfer_count = 24 * half_transfer_buffer_size;           // half dma transfer count
uint32 const src_bytes_per_half_transfer = 3 * half_transfer_buffer_size;    // each half, this many source bytes consumed
uint32 const all_channels_mask = (1 << (max_strings + 1)) - 1;
uint32 const gpio_data_on = all_channels_mask;     // set into BSRR to set outputs
uint16 const gpio_data_off = all_channels_mask;    // set into BRR to clear outputs

int which_half = 0;
int total_transferred;              // init to transfer_buffer_size after setting up buffer first time
byte *current_pixel_source;         // init to start of RGB buffer
byte frame_buffer[buffer_size];     // set colors in here (1st 3 bytes are RGB for led 1 string 1, then led 2 string 1, then led N string 1 then led 1 string 2 etc
byte dma_buffer[transfer_count];    // the dma buffer
byte *dma_buffer_pointer[2] = { dma_buffer, dma_buffer + half_transfer_count };    // pointers to each half (1st or 2nd half)

color * const color_buffer = (color *)frame_buffer;    // 200

void clear_spi_request();
void issue_spi_request();

spi_data_buffer spi_transmit_data;
spi_data_buffer spi_receive_data;

//////////////////////////////////////////////////////////////////////

void set_debug_led(bool on)
{
    uint32 mask = 1 << 5;   // B5 is debug led
    if(on) {
        mask <<= 16;
    }
    GPIOB->BSRR = mask;
}

//////////////////////////////////////////////////////////////////////

void system_clock_init()
{
    // oscillator
    RCC_OscInitTypeDef osc;
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    osc.HSIState = RCC_HSI_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL = RCC_PLL_MUL9;    // 72MHz
    HAL_RCC_OscConfig(&osc);

    // SYS, AHB, APB1 and APB2 clocks
    RCC_ClkInitTypeDef clk;
    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_1);

    // USB peripheral clock
    RCC_PeriphCLKInitTypeDef pclk;
    pclk.PeriphClockSelection = RCC_PERIPHCLK_USB;
    pclk.UsbClockSelection = RCC_USBCLKSOURCE_PLL;
    HAL_RCCEx_PeriphCLKConfig(&pclk);

    // systick
    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 1000);
    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
    HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

//////////////////////////////////////////////////////////////////////

extern "C" void HAL_SYSTICK_Callback()
{
    if(spi_pulse_delay != 0) {
        if(--spi_pulse_delay == 0) {
            clear_spi_request();
        }
    }
}

//////////////////////////////////////////////////////////////////////
// init the GPIOs

void gpio_init()
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_SPI2_CLK_ENABLE();
    __HAL_RCC_USART3_CLK_ENABLE();
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_TIM4_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    GPIO_InitTypeDef gpio;
    clear(gpio);

    // A0..A5, up to 6 strings of LEDs on PORTA
    gpio.Pin = (1 << max_strings) - 1; //GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    // A6 - ADC temp sensor
    gpio.Pin = GPIO_PIN_6;
    gpio.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &gpio);
    
    // B5 debug LED1
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pin = GPIO_PIN_5;
    HAL_GPIO_Init(GPIOB, &gpio);

    set_debug_led(true);

    // B6 pulse LED2
    gpio.Pin = GPIO_PIN_6;
    gpio.Mode = GPIO_MODE_AF_PP;
    HAL_GPIO_Init(GPIOB, &gpio);

    // B4 - REQUEST
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pin = GPIO_PIN_4;
    HAL_GPIO_Init(GPIOB, &gpio);
    clear_spi_request();

    // SPI2 GPIO Configuration    
    // PB12     ------> SPI2_NSS    INPUT
    // PB13     ------> SPI2_SCK    INPUT
    // PB14     ------> SPI2_MISO   AF_PP
    // PB15     ------> SPI2_MOSI   INPUT

    gpio.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_15;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &gpio);
    
    gpio.Pin = GPIO_PIN_14;
    gpio.Mode = GPIO_MODE_AF_PP;
    HAL_GPIO_Init(GPIOB, &gpio);
    
    // B10,11 - uart3

    gpio.Pin = GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_AF_PP;
    HAL_GPIO_Init(GPIOB, &gpio);

    gpio.Pin = GPIO_PIN_11;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &gpio);
}

//////////////////////////////////////////////////////////////////////

void tim4_Init()
{
    __HAL_RCC_TIM4_CLK_ENABLE();
    
    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 0;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 4096;
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim4);

    TIM_OC_InitTypeDef sConfigOC;
    clear(sConfigOC);

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_ENABLE;

    sConfigOC.Pulse = 2048;
    HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_1);

    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
}

//////////////////////////////////////////////////////////////////////

void irq_init()
{
    HAL_NVIC_SetPriority(SPI2_IRQn, 0, 15);             // SPI -> ESP12
    HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 15);    // SPI -> ESP12
    HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 15);    // SPI -> ESP12
    HAL_NVIC_SetPriority(TIM2_IRQn, 0, 15);             // 60Hz frame
    HAL_NVIC_SetPriority(TIM1_UP_IRQn, 0, 0);           // LEDs
    HAL_NVIC_SetPriority(ADC1_2_IRQn, 0, 15);           // Temp sensor
    HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);     // LEDs
    HAL_NVIC_SetPriority(USART3_IRQn, 0, 0);            // Debug uart

    HAL_NVIC_EnableIRQ(SPI2_IRQn);
    HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
    HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
    HAL_NVIC_EnableIRQ(TIM1_UP_IRQn);
    HAL_NVIC_EnableIRQ(ADC1_2_IRQn);
    HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
}

//////////////////////////////////////////////////////////////////////

void adc1_init(void)
{
    ADC_ChannelConfTypeDef sConfig;

    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    HAL_ADC_Init(&hadc1);
    sConfig.Channel = ADC_CHANNEL_6;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_41CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

//////////////////////////////////////////////////////////////////////
// setup DMA and associated timer

void dma_timer_init()
{
    // rather than use BSS, put these on the stack and clear them manually
    TIM_HandleTypeDef timer3;
    TIM_OC_InitTypeDef output_channel1;
    TIM_OC_InitTypeDef output_channel3;

    TIM3->CR1 &= ~TIM_CR1_CEN;
    TIM3->DIER &= ~(TIM_DMA_UPDATE | TIM_DMA_CC1 | TIM_DMA_CC3);

    clear(timer3);
    clear(output_channel1);
    clear(output_channel3);

    timer3.Instance = TIM3;

    timer3.Init.Period = timer3_period;
    HAL_TIM_PWM_Init(&timer3);

    // Channel 1 compare 29 = ~1/3rd of the period
    output_channel1.OCMode = TIM_OCMODE_PWM1;
    output_channel1.Pulse = off_1_period;
    HAL_TIM_PWM_ConfigChannel(&timer3, &output_channel1, TIM_CHANNEL_1);

    // Channel 2 compare 59 = ~2/3rds of the period
    output_channel3.OCMode = TIM_OCMODE_PWM1;
    output_channel3.Pulse = off_2_period;
    HAL_TIM_PWM_ConfigChannel(&timer3, &output_channel3, TIM_CHANNEL_3);

    TIM3->CCER = (TIM_CCx_ENABLE << TIM_CHANNEL_1) | (TIM_CCx_ENABLE << TIM_CHANNEL_3);
}

//////////////////////////////////////////////////////////////////////

static void spi2_init(void)
{
    // SPI2 DMA Init
    // SPI2_RX Init
    hdma_spi2_rx.Instance = DMA1_Channel4;
    hdma_spi2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_spi2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi2_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_spi2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_spi2_rx.Init.Mode = DMA_NORMAL;
    hdma_spi2_rx.Init.Priority = DMA_PRIORITY_MEDIUM;
    HAL_DMA_Init(&hdma_spi2_rx);

    __HAL_LINKDMA(&hspi2,hdmarx,hdma_spi2_rx);

    // SPI2_TX Init
    hdma_spi2_tx.Instance = DMA1_Channel5;
    hdma_spi2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_spi2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi2_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_spi2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_spi2_tx.Init.Mode = DMA_NORMAL;
    hdma_spi2_tx.Init.Priority = DMA_PRIORITY_MEDIUM;
    HAL_DMA_Init(&hdma_spi2_tx);

    __HAL_LINKDMA(&hspi2,hdmatx,hdma_spi2_tx);

    // SPI2 Init
    hspi2.Instance = SPI2;
    hspi2.Init.Mode = SPI_MODE_SLAVE;
    hspi2.Init.Direction = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi2.Init.NSS = SPI_NSS_HARD_INPUT;
    hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi2.Init.CRCPolynomial = 10;
    HAL_SPI_Init(&hspi2);
    
    clear_spi_request();
}

//////////////////////////////////////////////////////////////////////

void uart3_init()
{
    uart3.Instance = USART3;
    uart3.Init.BaudRate = 115200;
    uart3.Init.WordLength = UART_WORDLENGTH_8B;
    uart3.Init.StopBits = UART_STOPBITS_1;
    uart3.Init.Parity = UART_PARITY_NONE;
    uart3.Init.Mode = UART_MODE_TX_RX;
    uart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart3.Init.OverSampling = UART_OVERSAMPLING_16;
    uart3.pTxBuffPtr = uart3_transmit_buffer;
    uart3.TxXferSize = countof(uart3_transmit_buffer);
    HAL_UART_Init(&uart3);
    HAL_UART_Receive_IT(&uart3, uart3_receive_buffer, sizeof(uart3_receive_buffer));
}

//////////////////////////////////////////////////////////////////////

extern "C" void HAL_SPI_TxRxHalfCpltCallback(SPI_HandleTypeDef *hspi)
{
    clear_spi_request();
}

//////////////////////////////////////////////////////////////////////

extern "C" void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    spi_data = true;
}

//////////////////////////////////////////////////////////////////////

void issue_spi_request()
{
    // in case there was a dangling one that got missed by the ESP12
    //HAL_SPI_DMAStop(&hspi2);

    // not sending anything (yet)
    spi_transmit_data.init(0);
    
    // kick off SPI receive
    HAL_SPI_TransmitReceive_DMA(&hspi2, spi_transmit_data.transfer_buffer(), spi_receive_data.transfer_buffer(), 1024);

    // notify ESP12 that we're ready for SPI data
    CLR_GPIO(GPIOB, GPIO_PIN_4);
    
    spi_pulse_delay = 2;    // leave this high for between 1 and 2 ms (or until spi transfer is halfway done)
}

//////////////////////////////////////////////////////////////////////

void clear_spi_request()
{
    SET_GPIO(GPIOB, GPIO_PIN_4);
}

//////////////////////////////////////////////////////////////////////
// bitband some RGB24 bytes into bit stripes in the DMA buffer

void set_dma_pixels(int n, byte *src, byte *dst)
{
    int bit = max_strings;
    volatile uint32 *bitbase = BITBAND_ADDR(dst, 0);
    do {
        byte *s = src;
        src += bytes_per_string;
        volatile uint32 *bb = bitbase++;
        int i = n;
        do {
            // clang-format off
            byte d = *s++;
            bb[7 * 8] = d;
            bb[6 * 8] = d >>= 1;
            bb[5 * 8] = d >>= 1;
            bb[4 * 8] = d >>= 1;
            bb[3 * 8] = d >>= 1;
            bb[2 * 8] = d >>= 1;
            bb[1 * 8] = d >>= 1;
            bb[0 * 8] = d >>= 1;
            d = *s++;
            bb[15 * 8] = d;
            bb[14 * 8] = d >>= 1;
            bb[13 * 8] = d >>= 1;
            bb[12 * 8] = d >>= 1;
            bb[11 * 8] = d >>= 1;
            bb[10 * 8] = d >>= 1;
            bb[9 * 8] =  d >>= 1;
            bb[8 * 8] =  d >>= 1;
            d = *s++;
            bb[23 * 8] = d;
            bb[22 * 8] = d >>= 1;
            bb[21 * 8] = d >>= 1;
            bb[20 * 8] = d >>= 1;
            bb[19 * 8] = d >>= 1;
            bb[18 * 8] = d >>= 1;
            bb[17 * 8] = d >>= 1;
            bb[16 * 8] = d >>= 1;
            // clang-format on
            bb += 24 * 8;
        } while(--i > 0);
    } while(--bit > 0);
}

//////////////////////////////////////////////////////////////////////
// call this @ 60Hz

void draw_frame()
{
    if(frames < 32) {
        uint32 color = gamma_correct(frames * 255 / 32);    // blue glow to start with
        for(int i=0; i<max_total_leds; ++i) {
            color_buffer[i] = color;
        }
    }
    else {
        effect::frame_update();
    }
}

//////////////////////////////////////////////////////////////////////
// setup a timer

void timer_setup(TIM_TypeDef *t, uint16 prescaler, uint16 period)
{
    TIM_HandleTypeDef timer;
    clear(timer);
    timer.Instance = t;
    timer.Init.Prescaler = prescaler;
    timer.Init.CounterMode = TIM_COUNTERMODE_UP;
    timer.Init.Period = period;
    timer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    timer.Init.RepetitionCounter = 0;
    HAL_TIM_Base_Init(&timer);
    __HAL_TIM_CLEAR_IT(&timer, TIM_IT_UPDATE);
}

//////////////////////////////////////////////////////////////////////
// start 60Hz frame timer

void frame_timer_init()
{
    timer_setup(TIM2, 39, 29999);
    TIM2->SR = 0;
    TIM2->DIER |= TIM_IT_UPDATE;
    TIM2->CR1 |= TIM_CR1_CEN;
}

//////////////////////////////////////////////////////////////////////
// set up 50uS reset timer

void reset_timer_init()
{
    timer_setup(TIM1, 0, 3599);     // 50us LED data line reset
}

//////////////////////////////////////////////////////////////////////
// kick off the dma1 transfer on channels 3,6,2 via timer3

void kick_dma()
{
    while(!can_kick) {    // wait for last one to complete
    }
    
    can_kick = 0;

    TIM3->CR1 = 0;     // disable timer3
    TIM3->CCER = 0;    // disable timer3 capture/compare
    TIM3->DIER = 0;    // disable all timer3 triggers

    buffer_free = 0;                                                   // source buffer is busy
    current_pixel_source = frame_buffer + 3 * transfer_buffer_size;    // next source will be here
    which_half = 0;                                                    // back to first half when we get the 1st half transfer interrupt
    set_dma_pixels(transfer_buffer_size, frame_buffer, dma_buffer);    // setup full buffer initially
    total_transferred = 0;                                             // done this many so far

    DMA1->IFCR = DMA_ISR_GIF6;

    DMA1_Channel3->CCR = 0; // Update
    DMA1_Channel6->CCR = 0; // CH1
    DMA1_Channel2->CCR = 0; // CH3

    DMA1_Channel3->CNDTR = transfer_count;
    DMA1_Channel6->CNDTR = transfer_count;
    DMA1_Channel2->CNDTR = transfer_count;

    DMA1_Channel3->CPAR = (uint32)&GPIOA->BSRR;
    DMA1_Channel6->CPAR = (uint32)&GPIOA->BRR;
    DMA1_Channel2->CPAR = (uint32)&GPIOA->BRR;

    DMA1_Channel3->CMAR = (uint32)&gpio_data_on;     // switch on (0000ffff -> BSRR) at t = 0
    DMA1_Channel6->CMAR = (uint32)dma_buffer;        // maybe switch off (DMA -> BRR) at t = 1/3rd
    DMA1_Channel2->CMAR = (uint32)&gpio_data_off;    // switch off (ffff -> BRR) at t = 2/3rds

    // channel 3, 32 bits no inc
    DMA1_Channel3->CCR = DMA_CCR_EN | (DMA_CCR_PL_0 | DMA_CCR_PL_1) | DMA_CCR_MSIZE_1 | DMA_CCR_PSIZE_1 | DMA_CCR_CIRC | DMA_CCR_DIR;

    // channel 6, 8 bits, with inc
    DMA1_Channel6->CCR = DMA_CCR_EN | (DMA_CCR_PL_0 | DMA_CCR_PL_1) | DMA_CCR_PSIZE_1 | DMA_CCR_CIRC | DMA_CCR_DIR | DMA_CCR_MINC;

    // channel 2, 16 bits, no inc, half and full transfer interrupts enabled
    DMA1_Channel2->CCR = DMA_CCR_EN | (DMA_CCR_PL_0 | DMA_CCR_PL_1) | DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_1 | DMA_CCR_CIRC | DMA_CCR_DIR | DMA_CCR_TCIE | DMA_CCR_HTIE;

    TIM3->DIER = TIM_DMA_UPDATE | TIM_DMA_CC1 | TIM_DMA_CC3;    // enable timer3 dma triggers
    TIM3->CCER = (TIM_CCx_ENABLE << TIM_CHANNEL_1) | (TIM_CCx_ENABLE << TIM_CHANNEL_3);

    TIM3->CNT = timer3_period * 10 / 100;    // 1st one is late because
    TIM3->CR1 |= TIM_CR1_CEN;                // switch on the timer
    GPIOA->BSRR = all_channels_mask;         // all high to start with (fake 1st update event)
}

//////////////////////////////////////////////////////////////////////
// wait for 60Hz tick

void wait_vsync()
{
    uint32 f = frames;
    while(f == frames) {
    }
}

//////////////////////////////////////////////////////////////////////
// ISR: DMA is halfway or complete

extern "C" void DMA1_Channel2_IRQHandler()
{
    total_transferred += half_transfer_buffer_size;    // update # of transferred pixels
    if(total_transferred >= max_leds) {                // more to do?
        GPIOA->BRR = all_channels_mask;                // no, quick , kill all channels, ~100ns blip at the end seems to be ignored
        DMA1_Channel3->CCR = 0;                        // kill the DMA
        DMA1_Channel6->CCR = 0;
        DMA1_Channel2->CCR = 0;
        TIM3->SR = 0;                   // stop timer3
        TIM3->CR1 &= ~TIM_CR1_CEN;      //

        TIM1->SR = 0;                   // start timer1
        TIM1->DIER |= TIM_IT_UPDATE;    //
        TIM1->CR1 |= TIM_CR1_CEN;       //
        buffer_free = 1;                // release the frame buffer
    } else {
        set_dma_pixels(half_transfer_buffer_size, current_pixel_source, dma_buffer_pointer[which_half]);    // init the other half
        current_pixel_source += src_bytes_per_half_transfer;                                                // skip across the ones we just did
        which_half ^= 1;                                                                                    // toggle the other half flag
    }
    DMA1->IFCR = DMA_ISR_GIF2;    // clear Channel 2 IRQ
}

//////////////////////////////////////////////////////////////////////
// ISR: reset timer (1) is complete, set flag to allow further transfers

extern "C" void TIM1_UP_IRQHandler()
{
    TIM1->CR1 &= ~TIM_CR1_CEN;
    TIM1->DIER &= ~TIM_IT_UPDATE;
    TIM1->SR = 0;
    can_kick = 1;
}

//////////////////////////////////////////////////////////////////////
// ISR: 60Hz frame timer

extern "C" void TIM2_IRQHandler()
{
    TIM2->SR = 0;
    frames += 1;
}

//////////////////////////////////////////////////////////////////////

extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart == &uart3 && huart->pRxBuffPtr) {
        byte *d = command_buffer;
        byte *s = uart3_receive_buffer;
        int l = huart->pRxBuffPtr - uart3_receive_buffer;
        while(l-- != 0) {
            if(*s == 0x0d) {
                break;
            }
            *d++ = tolower(*s++);
        }
        *d++ = 0;
        command_flag = true;
    }
}

//////////////////////////////////////////////////////////////////////

extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    int new_reading = HAL_ADC_GetValue(hadc);
    adc_current_value = (adc_current_value * 3 / 4) + new_reading;
    adc_value = adc_current_value / 4;
    adc_flag = false;
}

//////////////////////////////////////////////////////////////////////

extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    uart_busy = huart->TxXferCount;
}

//////////////////////////////////////////////////////////////////////

void set_globals(message::global_state &state);

namespace
{
    using namespace message;

    //////////////////////////////////////////////////////////////////////

    struct led_handler : tracked_state<led_state>
    {
        virtual void on_data_arrived(data_buffer &din, data_buffer &dout)
        {
            effect::setup(state);
        }
    };

    //////////////////////////////////////////////////////////////////////

    struct stm_handler : tracked_state<stm_state>
    {
    };

    //////////////////////////////////////////////////////////////////////

    struct global_handler : tracked_state<global_state>
    {
        virtual void on_data_arrived(data_buffer &din, data_buffer &dout) 
        {
            set_globals(state);
        }
    };

    //////////////////////////////////////////////////////////////////////

    led_handler     led_state0;
    led_handler     led_state1;
    led_handler     led_state2;
    led_handler     led_state3;
    led_handler     led_state4;
    led_handler     led_state5;
    stm_handler     stm_state0;
    global_handler  global_state0;

    //////////////////////////////////////////////////////////////////////

    enum { num_channels = 8 };
    enum { num_led_channels = 6 };

    static state_handler *handlers[num_channels] = 
    {
        &led_state0,
        &led_state1,
        &led_state2,
        &led_state3,
        &led_state4,
        &led_state5,
        &stm_state0,
        &global_state0
    };
    
    byte dd[64];
    data_buffer dout(dd, 64);

    //////////////////////////////////////////////////////////////////////

    int handle_packet(data_buffer &din, data_buffer &dout)
    {
        dout.clear();
        message_header m;
        while (din.position < din.length)
        {
            din.take(&m);
            if(m.address <= 7) {
                handlers[m.address]->handle(m, din, dout);
            }
            else {
                din.position += m.size;
            }
        }
        return dout.position;
    }

    //////////////////////////////////////////////////////////////////////

    void update_spi()
    {
        if(spi_data) {
            spi_data = false;
            if(spi_receive_data.is_valid()) {
                data_buffer din(spi_receive_data.packet_buffer(), spi_receive_data.length);
                din.length = spi_receive_data.length;
                handle_packet(din, dout);
            }
            else {
                spi_errors += 1;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////

struct command_descriptor
{
    char const *name;
    void (*function)(int argc, char const **argv);
    char const *description;
};

//////////////////////////////////////////////////////////////////////

void reset_effects()
{
    for(int i=0; i<max_strings; ++i) {
        set_effect_desc(i, null);
    }
}

//////////////////////////////////////////////////////////////////////

void set_color(uint32 c)
{
    for(int i=0; i<max_total_leds; ++i) {
        color_buffer[i] = gamma_correct(c);
    }
}

#define PROMPT "\n>"

//////////////////////////////////////////////////////////////////////

void cmd_set_color(int argc, char const **argv)
{
    reset_effects();
    uint32 color = atohex(argv[1]);
    set_color(color);
    printf("I set it to %06x!"PROMPT, color);
}

//////////////////////////////////////////////////////////////////////

void cmd_show_frames(int argc, char const **argv)
{
    printf("%d frames"PROMPT, frames);
}

//////////////////////////////////////////////////////////////////////

int get_temp()
{
    return frames < 20 ? 0 : adc_value;
}

//////////////////////////////////////////////////////////////////////

void show_temp()
{
    printf("Frame: %5d, ADC = %5d"PROMPT, frames, get_temp());
}

//////////////////////////////////////////////////////////////////////

void cmd_do_temp(int argc, char const **argv)
{
    if(argc > 1) {
        int l = atoi(argv[1]);
        if(l > 3 && l <= 120) {
            printf("Temp loop %d frames"PROMPT, l);
            adc_delay = l;
            adc_loop = l;
            return;
        }
        else {
            printf("Frame delay must be >3 and <= 120\n");
        }
    }
    show_temp();
}

//////////////////////////////////////////////////////////////////////

void cmd_do_spi(int argc, char const **argv)
{
    byte *d = spi_transmit_data.packet_buffer();
    for(int i=0; i<spi_data_buffer::packet_size; ++i) {
        d[i] = i & 0xff;
    }
    spi_transmit_data.init(spi_data_buffer::packet_size);
    printf("Here we go, sending %04x,%04x"PROMPT, spi_transmit_data.length, spi_transmit_data.crc);
    issue_spi_request();
}

//////////////////////////////////////////////////////////////////////

extern command_descriptor commands[];

void cmd_help(int argc, char const **argv)
{
    command_descriptor *desc = commands;
    while(desc->name != null) {
        printf("%10s %s\n", desc->name, desc->description);
        desc += 1;
    }
    printf(PROMPT);
}

//////////////////////////////////////////////////////////////////////

void cmd_do_leds(int argc, char const **argv)
{
//    printf("%d %d\n%d %d\n%d %d\n%d %d\n%d %d\n%d %d\n",
//        0, ((led_state *)&led_state0.state)->effect_index,
//        1, ((led_state *)&led_state1.state)->effect_index,
//        2, ((led_state *)&led_state2.state)->effect_index,
//        3, ((led_state *)&led_state3.state)->effect_index,
//        4, ((led_state *)&led_state4.state)->effect_index,
//        5, ((led_state *)&led_state5.state)->effect_index);


//     printf("%d,%d\n>", ((led_state *)&led_state5.state)->speed_ranges[0].low,((led_state *)&led_state5.state)->speed_ranges[0].high);
    if(argc > 1) {
        int s = atoi(argv[1]);
        effect *eff = get_effect_buffer(s);
        ::color l, h;
        ::color_range &r = eff->color_ranges[0];
        memcpy((byte *)&l, (byte *)&r.low, 3);
        memcpy((byte *)&h, (byte *)&r.high, 3);
        
        effect_desc *desc = get_effect_descriptor(9);

        printf("%d: effect %08x, %08x,%08x %d,%d (%08x)"PROMPT, s, eff->current_desc, (uint32)l,(uint32)h, eff->speed_ranges[0].low, eff->speed_ranges[0].high, desc);
    }
    else {
        printf("usage: leds #"PROMPT);
    }
}

//////////////////////////////////////////////////////////////////////

command_descriptor commands[] =
{
    { "help", cmd_help, "Show this list" },
    { "color", cmd_set_color, "Set all leds to color RRGGBB" },
    { "frames", cmd_show_frames, "Show frame counter" },
    { "temp", cmd_do_temp, "Show temp every N frames (3 < N <= 120)" },
    { "spi", cmd_do_spi, "Send an SPI request" },
    { "leds", cmd_do_leds, "Show LED string data" },
    { null, null }
};

//////////////////////////////////////////////////////////////////////

void handle_command()
{
    if(command_flag) {
        adc_delay = 0;
        adc_loop = 0;

        char arg_buffer[128];
        int argc = 0;
        char *argv[8];

        char const *src = (char const *)command_buffer;

        char *dest = arg_buffer;
        char *arg_start = null;
        
        clear(argv);

        while(true) {
            char c = *src++;
            if(c == ' ' || c == 0) {
                if(dest != arg_start) {
                    argv[argc++] = arg_start;
                    *dest++ = 0;
                    arg_start = null;
                }
                if(c == 0) {
                    break;
                }
            }
            else {
                if(arg_start == null) {
                    arg_start = dest;
                }
                *dest++ = c;
            }
        }
        
        command_descriptor *cmd = null;
        command_descriptor *desc = commands;
        if(argv[0] && argv[0][0]) {
            while(desc->name != null) {
                if(strcmp(desc->name, argv[0]) == 0) {
                    cmd = desc;
                    break;
                }
                desc += 1;
            }
            if(cmd != null) {
                cmd->function(argc, (char const **)argv);
            }
            else {
                printf("??"PROMPT);
            }
        }
        else {
            printf(PROMPT);
        }
        command_flag = 0;
        HAL_UART_Receive_IT(&uart3, uart3_receive_buffer, sizeof(uart3_receive_buffer));
    }
}

//////////////////////////////////////////////////////////////////////

void update_adc()
{
    if(adc_delay != 0) {
        if(--adc_loop == 0) {
            show_temp();
            adc_loop = adc_delay;
        }
    }
    if(!adc_flag) {
        adc_flag = true;
        HAL_ADC_Start_IT(&hadc1);
    }
}

//////////////////////////////////////////////////////////////////////

void set_pulse_led(int n)
{
    TIM4->CCR1 = 4095 - (n & 4095);
}

//////////////////////////////////////////////////////////////////////
// main

int main()
{
    int p = 0;
    int q = 0;
    
    global_state0.state.flags = global_power;
    global_state0.state.brightness = 255;
    global_state0.state.speed = 128;
    global_state0.state.flags |= message::global_debug_led;

    HAL_Init();
    system_clock_init();
    gpio_init();
    adc1_init();
    spi2_init();
    uart3_init();
    dma_timer_init();
    reset_timer_init();
    frame_timer_init();
    irq_init();
    effect::reset();
    tim4_Init();
    
    printf("\n====================\nReady:"PROMPT);

    set_pulse_led(64);

    while(1) {
        draw_frame();
        wait_vsync();
        kick_dma();
        
        // this is lame, wait for the led dma to complete before doing the SPI transfer
        while(!can_kick) {
        }
        
        update_spi();
        issue_spi_request();
        update_adc();
        handle_command();

        p += 32;
        int s = sin(p) * 8 + 2047;
        //set_pulse_led(s);
        
        q = (q + 5) & 255;
        set_debug_led(global_state0.state.flags & message::global_debug_led);
    }
}
