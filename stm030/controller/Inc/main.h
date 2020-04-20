//////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////

#include "stm32f0xx_hal.h"

//////////////////////////////////////////////////////////////////////

typedef unsigned char uint8;
typedef unsigned char byte;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef unsigned int uint;

typedef signed char int8;
typedef signed short int16;
typedef signed int int32;
typedef signed long long int64;

//////////////////////////////////////////////////////////////////////

void Error_Handler(void);
extern uint64_t const five_1[256];

extern SPI_HandleTypeDef hspi1;
extern DMA_HandleTypeDef hdma_spi1_tx;
extern TIM_HandleTypeDef htim14;

#if 0
#define SET(x) (GPIOA->BSRR = (x))
#define CLR(x) (GPIOA->BRR = (x))
#define TOGGLE(x) (GPIOA->ODR ^= (x))
#else
#define SET(x)
#define CLR(x)
#define TOGGLE(x)
#endif

#define YELLOW (1 << 2)
#define BLUE (1 << 3)
#define PINK (1 << 5)
#define DARKBLUE (1 << 6)
