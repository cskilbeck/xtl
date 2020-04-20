#pragma once

#include "sdkconfig.h"

// wifi_core is where the (very noisy) wifi task lives
// put time sensitive tasks on the other core

#if defined(CONFIG_ESP32_WIFI_TASK_PINNED_TO_CORE_0)
constexpr int wifi_core = 0;
#elif defined(CONFIG_ESP32_WIFI_TASK_PINNED_TO_CORE_1)
constexpr int wifi_core = 1;
#else
#error "WIFI CORE TASK PINNING IS NOT DEFINED!?"
#endif

void initialise_wifi();
