#include "stm32f1xx_hal.h"

#ifdef HAL_MODULE_ENABLED

#define __STM32F1xx_HAL_VERSION_MAIN (0x01U) /*!< [31:24] main version */
#define __STM32F1xx_HAL_VERSION_SUB1 (0x01U) /*!< [23:16] sub1 version */
#define __STM32F1xx_HAL_VERSION_SUB2 (0x01U) /*!< [15:8]  sub2 version */
#define __STM32F1xx_HAL_VERSION_RC (0x00U)   /*!< [7:0]  release candidate */
#define __STM32F1xx_HAL_VERSION ((__STM32F1xx_HAL_VERSION_MAIN << 24) | (__STM32F1xx_HAL_VERSION_SUB1 << 16) | (__STM32F1xx_HAL_VERSION_SUB2 << 8) | (__STM32F1xx_HAL_VERSION_RC))

#define IDCODE_DEVID_MASK 0x00000FFFU

__IO uint32_t uwTick;
// This function is used to initialize the HAL Library; it must be the first
//         instruction to be executed in the main program (before to call any other
//         HAL function)
HAL_StatusTypeDef HAL_Init(void)
{
#if(PREFETCH_ENABLE != 0)
#if defined(STM32F101x6) || defined(STM32F101xB) || defined(STM32F101xE) || defined(STM32F101xG) || defined(STM32F102x6) || defined(STM32F102xB) || defined(STM32F103x6) || \
    defined(STM32F103xB) || defined(STM32F103xE) || defined(STM32F103xG) || defined(STM32F105xC) || defined(STM32F107xC)

    // Prefetch buffer is not available on value line devices
    __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
#endif
#endif /* PREFETCH_ENABLE */

    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    HAL_InitTick(TICK_INT_PRIORITY);
    HAL_MspInit();
    return HAL_OK;
}

HAL_StatusTypeDef HAL_DeInit(void)
{
    __HAL_RCC_APB1_FORCE_RESET();
    __HAL_RCC_APB1_RELEASE_RESET();
    __HAL_RCC_APB2_FORCE_RESET();
    __HAL_RCC_APB2_RELEASE_RESET();

#if defined(STM32F105xC) || defined(STM32F107xC)
    __HAL_RCC_AHB_FORCE_RESET();
    __HAL_RCC_AHB_RELEASE_RESET();
#endif

    HAL_MspDeInit();
    return HAL_OK;
}

__weak void HAL_MspInit(void)
{
}

__weak void HAL_MspDeInit(void)
{
}

__weak HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    HAL_SYSTICK_Config(SystemCoreClock / 1000U);
    HAL_NVIC_SetPriority(SysTick_IRQn, TickPriority, 0U);
    return HAL_OK;
}

__weak void HAL_IncTick(void)
{
    uwTick++;
}

__weak uint32_t HAL_GetTick(void)
{
    return uwTick;
}

__weak void HAL_Delay(__IO uint32_t Delay)
{
    uint32_t tickstart = HAL_GetTick();
    uint32_t wait = Delay;

    if(wait < HAL_MAX_DELAY) {
        wait++;
    }

    while((HAL_GetTick() - tickstart) < wait) {
    }
}

__weak void HAL_SuspendTick(void)
{
    CLEAR_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);
}

__weak void HAL_ResumeTick(void)
{
    SET_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);
}

uint32_t HAL_GetHalVersion(void)
{
    return __STM32F1xx_HAL_VERSION;
}

uint32_t HAL_GetREVID(void)
{
    return ((DBGMCU->IDCODE) >> DBGMCU_IDCODE_REV_ID_Pos);
}

uint32_t HAL_GetDEVID(void)
{
    return ((DBGMCU->IDCODE) & IDCODE_DEVID_MASK);
}

void HAL_DBGMCU_EnableDBGSleepMode(void)
{
    SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_SLEEP);
}

void HAL_DBGMCU_DisableDBGSleepMode(void)
{
    CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_SLEEP);
}

void HAL_DBGMCU_EnableDBGStopMode(void)
{
    SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_STOP);
}

void HAL_DBGMCU_DisableDBGStopMode(void)
{
    CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_STOP);
}

void HAL_DBGMCU_EnableDBGStandbyMode(void)
{
    SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_STANDBY);
}

void HAL_DBGMCU_DisableDBGStandbyMode(void)
{
    CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_STANDBY);
}

void HAL_GetUID(uint32_t *UID)
{
    UID[0] = (uint32_t)(READ_REG(*((uint32_t *)UID_BASE)));
    UID[1] = (uint32_t)(READ_REG(*((uint32_t *)(UID_BASE + 4U))));
    UID[2] = (uint32_t)(READ_REG(*((uint32_t *)(UID_BASE + 8U))));
}

#endif
