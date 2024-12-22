//////////////////////////////////////////////////////////////////////

#include "esp_system.h"
#include "driver/gpio.h"
#include "button.h"

#define BUTTON_GPIO_NUM GPIO_NUM_14
#define BUTTON_GPIO_PIN GPIO_Pin_14

//////////////////////////////////////////////////////////////////////

bool button_down = false;
bool button_last = false;
bool button_pressed = false;
bool button_released = false;

//////////////////////////////////////////////////////////////////////

void button_init()
{
    gpio_config_t c;
    c.pin_bit_mask = BUTTON_GPIO_PIN;
    c.mode = GPIO_MODE_INPUT;
    c.pull_up_en = GPIO_PULLUP_ENABLE;
    c.pull_down_en = GPIO_PULLDOWN_DISABLE;
    c.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&c);
}

//////////////////////////////////////////////////////////////////////

void button_update()
{
    button_down = gpio_get_level(BUTTON_GPIO_NUM) == 0;
    bool button_changed = button_down != button_last;
    button_last = button_down;
    button_pressed = button_changed & button_down;
    button_released = button_changed & !button_down;
}
