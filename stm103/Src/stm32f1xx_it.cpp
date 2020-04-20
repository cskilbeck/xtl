#include "stm32f1xx_hal.h"
#include "stm32f1xx.h"
#include "stm32f1xx_it.h"

extern void flash(int on, int off);

extern "C" void NMI_Handler(void)
{
}

extern "C" void HardFault_Handler(void)
{
    flash(1000000, 500000);
}

extern "C" void MemManage_Handler(void)
{
    flash(1000000, 500000);
}

extern "C" void BusFault_Handler(void)
{
    flash(1000000, 500000);
}

extern "C" void UsageFault_Handler(void)
{
    flash(1000000, 500000);
}

extern "C" void SVC_Handler(void)
{
}

extern "C" void DebugMon_Handler(void)
{
}

extern "C" void PendSV_Handler(void)
{
}

extern "C" void SysTick_Handler(void)
{
    HAL_IncTick();
    HAL_SYSTICK_IRQHandler();
}

//////////////////////////////////////////////////////////////////////

extern ADC_HandleTypeDef hadc1;
extern UART_HandleTypeDef uart3;
extern SPI_HandleTypeDef hspi2;
extern DMA_HandleTypeDef hdma_spi2_rx;
extern DMA_HandleTypeDef hdma_spi2_tx;

//////////////////////////////////////////////////////////////////////

extern "C" void SPI2_IRQHandler()
{
    HAL_SPI_IRQHandler(&hspi2);
}

//////////////////////////////////////////////////////////////////////

extern "C" void ADC1_2_IRQHandler(void)
{
    HAL_ADC_IRQHandler(&hadc1);
}

//////////////////////////////////////////////////////////////////////

extern "C" void USART3_IRQHandler()
{
    HAL_UART_IRQHandler(&uart3);
}

//////////////////////////////////////////////////////////////////////

extern "C" void DMA1_Channel4_IRQHandler()
{
    HAL_DMA_IRQHandler(&hdma_spi2_rx);
}

//////////////////////////////////////////////////////////////////////

extern "C" void DMA1_Channel5_IRQHandler()
{
    HAL_DMA_IRQHandler(&hdma_spi2_tx);
}
