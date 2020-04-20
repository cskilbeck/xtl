
#include "stm32f1xx_hal.h"
#include "types.h"
#include "main.h"
#include "util.h"

extern DMA_HandleTypeDef hdma_spi2_rx;
extern DMA_HandleTypeDef hdma_spi2_tx;

//////////////////////////////////////////////////////////////////////

void HAL_MspInit(void)
{
    __HAL_RCC_AFIO_CLK_ENABLE();
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

    // System interrupt init
    HAL_NVIC_SetPriority(MemoryManagement_IRQn, 0, 0);    // MemoryManagement_IRQn interrupt configuration
    HAL_NVIC_SetPriority(BusFault_IRQn, 0, 0);            // BusFault_IRQn interrupt configuration
    HAL_NVIC_SetPriority(UsageFault_IRQn, 0, 0);          // UsageFault_IRQn interrupt configuration
    HAL_NVIC_SetPriority(SVCall_IRQn, 0, 0);              // SVCall_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DebugMonitor_IRQn, 0, 0);        // DebugMonitor_IRQn interrupt configuration
    HAL_NVIC_SetPriority(PendSV_IRQn, 0, 0);              // PendSV_IRQn interrupt configuration
    HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);             // SysTick_IRQn interrupt configuration
    __HAL_AFIO_REMAP_SWJ_NOJTAG();                        // NOJTAG: JTAG-DP Disabled and SW-DP Enabled
}

