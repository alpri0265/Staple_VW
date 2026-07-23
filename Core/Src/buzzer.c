#include "buzzer.h"
#include "main.h"
#include "stm32f4xx_hal.h"

static bool     s_active = false;
static uint32_t s_deadline = 0U;

static void buzzer_write(GPIO_PinState state)
{
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, state);
}

void buzzer_init(void)
{
    buzzer_write(BUZZER_IDLE_STATE);
    s_active = false;
    s_deadline = 0U;
}

void buzzer_beep(uint16_t ms)
{
    if (ms == 0U) {
        buzzer_stop();
        return;
    }

    buzzer_write(BUZZER_ACTIVE_STATE);
    s_active = true;
    s_deadline = HAL_GetTick() + (uint32_t)ms;
}

void buzzer_stop(void)
{
    buzzer_write(BUZZER_IDLE_STATE);
    s_active = false;
    s_deadline = 0U;
}

bool buzzer_is_active(void)
{
    return s_active;
}

void buzzer_update(void)
{
    if (!s_active) return;
    if ((int32_t)(HAL_GetTick() - s_deadline) >= 0) {
        buzzer_stop();
    }
}

