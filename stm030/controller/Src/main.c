//////////////////////////////////////////////////////////////////////

#include "main.h"
#include "color.h"
#include "util.h"
#include "spi_leds.h"
#include "ir_decode.h"

//////////////////////////////////////////////////////////////////////

void SystemClock_Config(void);
static void MX_SPI_Init(void);
static void MX_GPIO_Init(void);
static void MX_TIM_Init(void);

void Stack_Error(void);

//////////////////////////////////////////////////////////////////////

SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_tx;
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim14;

//////////////////////////////////////////////////////////////////////

color colors[3];
int next_color = 0;

int flash = 0;

extern uint32 Stack_Mem;
extern uint32 Stack_End;

color const color_red = { 0xff, 0, 0 };
color const color_green = { 0, 0xff, 0 };
color const color_blue = { 0, 0, 0xff };
color const color_white = { 0xff, 0xff, 0xff };

enum
{
    fx_flash,
    fx_fade,
    fx_smooth
};

int speed = 1024;
int ramp = 1024;
int imul = 0;
int effect = fx_flash;

//////////////////////////////////////////////////////////////////////
// a color button was pushed, add to the cycle

void push_color(color c)
{
    if(ir_repeat == 0) {
        power = 1;
        flash = 4;
        colors[next_color] = c;
        next_color += 1;
        if(next_color == 3) {
            next_color = 0;
        }
    }
}

//////////////////////////////////////////////////////////////////////
// clamped triangle wave
// returns 0..256 for lerp between two colors

int fade_wave(int x, int speed, int ramp)
{
    return min(max((abs(((x * speed >> 8) & 511) - 256) - 128) * ramp >> 8, -128), 128) + 128;
}

//////////////////////////////////////////////////////////////////////
// some effect button pressed
// cycle through speeds, reset speed if new effect

void set_effect(int eff)
{
    if(effect != eff) {
        speed = 512;
    } else if(ir_repeat == 0) {
        speed = speed * 2;
        if(speed == 8192) {
            speed = 512;
        }
    }
    effect = eff;
}

//////////////////////////////////////////////////////////////////////
// deal with ir decoded keypress
// ir_repeat is # of repeats, will be 0 for initial press

void handle_key(int key)
{
    switch(key) {

    case key_off:
        power = 0;
        break;

    case key_on:
        power = 1;
        break;

    case key_bright_down:
        change_brightness(-16);
        break;

    case key_bright_up:
        change_brightness(16);
        break;

    case key_flash:
        set_effect(fx_flash);
        ramp = 8192;
        imul = 229;
        break;

    case key_fade:
        set_effect(fx_fade);
        effect = fx_fade;
        ramp = 512;
        imul = 0;
        break;

    case key_smooth:
        set_effect(fx_fade);
        effect = fx_fade;
        ramp = 384;
        imul = 3;
        break;

    case key_red:
        push_color(color_red);
        break;

    case key_green:
        push_color(color_green);
        break;

    case key_blue:
        push_color(color_blue);
        break;

    case key_white:
        push_color(color_white);
        break;
    }
}

//////////////////////////////////////////////////////////////////////
// render into current back buffer

void draw_frame()
{
    color *rgb_buffer = (color *)current_frame_buffer();
    color_update();

    if(flash != 0) {
        flash -= 1;
        uint32 *p = (uint32 *)rgb_buffer;
        for(int i = 0; i < NUM_LED_DWORDS; ++i) {
            p[i] = 0x0f0f0f;
        }
    } else {
        color c[2];
        for(int i = 0; i < NUM_LEDS; ++i) {
            int x = frames + i * imul;
            int l = fade_wave(x, speed, ramp);
            int p = (x * speed) >> 16;
            int p1 = p & 1;
            uint32 p2 = p % 3;
            int p3 = p2 + 1;
            if(p3 > 2) {
                p3 = 0;
            }
            c[p1] = colors[p3];
            c[1 - p1] = colors[p2];
            color a = gamma_correct(color_lerp(c[0], c[1], l));
            rgb_buffer[i] = a;
        }
    }
}

//////////////////////////////////////////////////////////////////////

int main(void)
{
    volatile uint32 *stack = &Stack_Mem;
    do {
        *stack++ = 0xcccccccc;
    } while(stack < &Stack_End);

    HAL_Init();

    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    SystemClock_Config();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_TIM14_CLK_ENABLE();

    MX_GPIO_Init();
    MX_SPI_Init();
    MX_TIM_Init();

    HAL_SuspendTick();

    HAL_TIM_PWM_Start(&htim14, TIM_CHANNEL_1);
    HAL_TIM_OC_Start(&htim14, TIM_CHANNEL_2);

    TIM1->DIER |= TIM_IT_UPDATE | TIM_IT_TRIGGER | TIM_IT_CC1 | TIM_IT_CC3;
    TIM1->CCER |= TIM_CCER_CC1E | TIM_CCER_CC3E;
    TIM1->CR1 = TIM_CR1_OPM;

    colors[0] = color_from_rgb(0xff, 0, 0);
    colors[1] = color_from_rgb(0, 0xff, 0);
    colors[2] = color_from_rgb(0, 0, 0xff);

    while(1) {

        spi_leds_send();

        if(Stack_Mem != 0xcccccccc) {
            Stack_Error();
        }

        if(ir_press == 0) {
            TIM14->CCR1 = 0;    // debug led off
        } else {
            ir_press = 0;
            if(ir_repeat == 0) {
                TIM14->CCR1 = 12000;    // debug led bright flash for 1 frame when key pressed
            } else {
                TIM14->CCR1 = 500;    // debug led dim flash when key repeat
            }
            handle_key(ir_key);
        }
        draw_frame();
    }
}

//////////////////////////////////////////////////////////////////////
// TIM1 ir decode trigger/update isr

void TIM1_BRK_UP_TRG_COM_IRQHandler(void)
{
    // 1st falling edge after a long while
    if(TIM1->SR & TIM_FLAG_TRIGGER) {
        TOGGLE(YELLOW);
        TIM1->SR = ~TIM_FLAG_TRIGGER;
        TIM1->DIER &= ~TIM_IT_TRIGGER;
    }
    // nothing happened for a long while
    if(TIM1->SR & TIM_FLAG_UPDATE) {
        TOGGLE(BLUE);
        TIM1->SR = ~TIM_FLAG_UPDATE;
        TIM1->DIER |= TIM_IT_TRIGGER;
        ir_reset();
        ir_key = 0;
    }
}

//////////////////////////////////////////////////////////////////////
// TIM1 ir decode input capture / counter compare

void TIM1_CC_IRQHandler(void)
{
    // input capture - any falling edge
    if(TIM1->SR & TIM_FLAG_CC3) {
        TIM1->CNT = 0;
        TOGGLE(DARKBLUE);
        ir_falling_edge(TIM1->CCR3);
        TIM1->SR = ~TIM_FLAG_CC3;
    }
    // counter compare - nothing happened for a short while
    if(TIM1->SR & TIM_FLAG_CC1) {
        TOGGLE(PINK);
        ir_reset();
        TIM1->SR = ~TIM_FLAG_CC1;
    }
}

//////////////////////////////////////////////////////////////////////

void SystemClock_Config(void)
{
    {
        RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
        RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
        RCC_OscInitStruct.HSIState = RCC_HSI_ON;
        RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
        RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
        RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
        RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
        RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
        HAL_RCC_OscConfig(&RCC_OscInitStruct);
    }

    {
        RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
        RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
        RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
        RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
        RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
        HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1);
    }
}

//////////////////////////////////////////////////////////////////////
// SPI for PL9823 rgb led data
// clock divider is 16, so 3MHz
// 5 SPI bits per RGB bit = 600KHz = 1.66uS per bit
// For (0/1) use (10000 / 11110)
// 0: 0.33uS high, 1.33uS low
// 1: 1.33uS high, 0.33uS low
// see stash_5() in spi_leds.c
// # of leds must be a multiple of 12 but you don't have to connect them all - extra data is ignored

static void MX_SPI_Init(void)
{
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_1LINE;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 7;
    hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    HAL_SPI_Init(&hspi1);

    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = GPIO_PIN_7;    // JUST MOSI (A7), not CLK (A5), we don't need it
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF0_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    hdma_spi1_tx.Instance = DMA1_Channel3;
    hdma_spi1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_spi1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi1_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_spi1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_spi1_tx.Init.Mode = DMA_CIRCULAR;
    hdma_spi1_tx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    HAL_DMA_Init(&hdma_spi1_tx);

    __HAL_LINKDMA(&hspi1, hdmatx, hdma_spi1_tx);

    HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
}

//////////////////////////////////////////////////////////////////////
// TIM1 - IR decode
// TIM14 - debug led pwm

static void MX_TIM_Init(void)
{
    // TIM1 ir decode
    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 479;
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = 12000;    // 120ms is a long while, do a full state reset when this wraps (so unexpected key repeats are ignored)
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&htim1);
    HAL_TIM_IC_Init(&htim1);

    // slavemode trigger, detect initial falling edge
    {
        TIM_SlaveConfigTypeDef sSlaveConfig = { 0 };
        sSlaveConfig.SlaveMode = TIM_SLAVEMODE_TRIGGER;
        sSlaveConfig.InputTrigger = TIM_TS_TI2FP2;
        sSlaveConfig.TriggerPolarity = TIM_TRIGGERPOLARITY_FALLING;
        sSlaveConfig.TriggerFilter = 15;
        HAL_TIM_SlaveConfigSynchro(&htim1, &sSlaveConfig);
    }

    // no master mode
    {
        TIM_MasterConfigTypeDef sMasterConfig = { 0 };
        sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
        sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
        HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig);
    }

    // input capture channel 1 for timings
    {
        TIM_IC_InitTypeDef sConfigIC = { 0 };
        sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
        sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
        sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
        sConfigIC.ICFilter = 15;
        HAL_TIM_IC_ConfigChannel(&htim1, &sConfigIC, TIM_CHANNEL_3);
    }

    // compare (no output) channel 1 for watchdog timer (reset when idle for 1.4ms)
    {
        TIM_OC_InitTypeDef sConfigOC = { 0 };
        sConfigOC.OCMode = TIM_OCMODE_TIMING;
        sConfigOC.Pulse = 1400;    // 1.4ms is a short while
        sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
        sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
        HAL_TIM_OC_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1);
    }

    // two pins! annoying. one for trigger, one for input capture, same connection
    {
        GPIO_InitTypeDef GPIO_InitStruct = { 0 };
        GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF2_TIM1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }

    // irqs for ir decode
    HAL_NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);
    HAL_NVIC_SetPriority(TIM1_CC_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM1_CC_IRQn);

    // TIM14 debug led pwm
    htim14.Instance = TIM14;
    htim14.Init.Prescaler = 0;
    htim14.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim14.Init.Period = 11999;
    htim14.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim14.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_Base_Init(&htim14);
    HAL_TIM_PWM_Init(&htim14);

    {
        TIM_OC_InitTypeDef sConfigOC = { 0 };
        sConfigOC.OCMode = TIM_OCMODE_PWM1;
        sConfigOC.Pulse = 1300;
        sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
        sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
        HAL_TIM_PWM_ConfigChannel(&htim14, &sConfigOC, TIM_CHANNEL_1);
    }

    {
        GPIO_InitTypeDef GPIO_InitStruct = { 0 };
        GPIO_InitStruct.Pin = GPIO_PIN_4;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF4_TIM14;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

//////////////////////////////////////////////////////////////////////
// GPIOA 2,3,5,6 - debug on scope
// GPIOA 4 - debug led on dev board

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    GPIOA->ODR = 0;

    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

//////////////////////////////////////////////////////////////////////
// Error_Handler - flash led

void Error_Handler(void)
{
    TIM14->CR1 = 0;
    GPIOA->BSRR = 1 << 4;
    for(int i = 0; i < 1000000; ++i) {
        __NOP();
    }
    GPIOA->BRR = 1 << 4;
    for(int i = 0; i < 1000000; ++i) {
        __NOP();
    }
}

//////////////////////////////////////////////////////////////////////
// Stack_Error - pulse led if stack is exhausted

void Stack_Error(void)
{
    int x = 0;
    while(1) {
        TIM14->CCR1 = x;
        x += 1;
        for(int i = 0; i < 500; ++i) {
            __NOP();
        }
        if(x > 12000) {
            x = 0;
        }
    }
}
