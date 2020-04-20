#include "stm32f0xx.h"

#if !defined(HSI48_VALUE)
#define HSI48_VALUE ((uint32_t)48000000)
#endif

uint32_t SystemCoreClock = 8000000;

const uint8_t AHBPrescTable[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9 };
const uint8_t APBPrescTable[8] = { 0, 0, 0, 0, 1, 2, 3, 4 };

void SystemCoreClockUpdate(void)
{
    SystemCoreClock = HSI48_VALUE;
}
