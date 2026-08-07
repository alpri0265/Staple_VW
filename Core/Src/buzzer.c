#include "buzzer.h"
#include "main.h"
#include "stm32f4xx_hal.h"

// Пасивний п'єзо мовчить від постійного DC — потрібен змінний сигнал.
// TIM3 тікає кожні 100мкс; перемикання піна раз на N тіків дає прямокутну
// хвилю: f = 5000 / N Гц. Резонанс маленьких п'єзо вузький і невідомий
// наперед, тому поки звучить — розгортаємо частоту від MAX до MIN тіків
// (тобто від низької до високої частоти) і назад, щоб гарантовано пройти
// через резонансний пік.
#define BUZZER_SWEEP_MIN_TICKS   1U   // 5000 Гц
#define BUZZER_SWEEP_MAX_TICKS   8U   // 625 Гц
#define BUZZER_SWEEP_STEP_MS     40U  // тривалість одного кроку розгортки

static bool     s_active           = false;
static bool     s_continuous       = false;
static uint32_t s_deadline         = 0U;
static uint16_t s_tone_counter     = 0U;
static uint16_t s_tone_toggle_ticks = BUZZER_SWEEP_MIN_TICKS;
static uint32_t s_sweep_step_tick  = 0U;
static int8_t   s_sweep_dir        = 1;

static void buzzer_write(GPIO_PinState state)
{
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, state);
}

static void buzzer_sweep_reset(void)
{
    s_tone_toggle_ticks = BUZZER_SWEEP_MIN_TICKS;
    s_sweep_dir         = 1;
    s_sweep_step_tick   = HAL_GetTick();
    s_tone_counter      = 0U;
}

void buzzer_init(void)
{
    buzzer_write(BUZZER_IDLE_STATE);
    s_active     = false;
    s_continuous = false;
    s_deadline   = 0U;
    buzzer_sweep_reset();
}

void buzzer_tim_tick(void)
{
    if (!s_active) {
        return;
    }
    if (s_tone_counter > 0U) {
        s_tone_counter--;
        return;
    }
    s_tone_counter = s_tone_toggle_ticks - 1U;
    HAL_GPIO_TogglePin(BUZZER_GPIO_Port, BUZZER_Pin);
}

void buzzer_beep(uint16_t ms)
{
    if (ms == 0U) {
        buzzer_stop();
        return;
    }

    buzzer_sweep_reset();
    s_active     = true;
    s_continuous = false;
    s_deadline   = HAL_GetTick() + (uint32_t)ms;
}

void buzzer_on(void)
{
    buzzer_sweep_reset();
    s_active     = true;
    s_continuous = true;
}

void buzzer_stop(void)
{
    buzzer_write(BUZZER_IDLE_STATE);
    s_active     = false;
    s_continuous = false;
    s_deadline   = 0U;
}

bool buzzer_is_active(void)
{
    return s_active;
}

void buzzer_update(void)
{
    if (!s_active) return;

    if ((HAL_GetTick() - s_sweep_step_tick) >= BUZZER_SWEEP_STEP_MS) {
        s_sweep_step_tick = HAL_GetTick();
        s_tone_toggle_ticks = (uint16_t)(s_tone_toggle_ticks + s_sweep_dir);
        if (s_tone_toggle_ticks >= BUZZER_SWEEP_MAX_TICKS) {
            s_tone_toggle_ticks = BUZZER_SWEEP_MAX_TICKS;
            s_sweep_dir = -1;
        } else if (s_tone_toggle_ticks <= BUZZER_SWEEP_MIN_TICKS) {
            s_tone_toggle_ticks = BUZZER_SWEEP_MIN_TICKS;
            s_sweep_dir = 1;
        }
    }

    if (s_continuous) return;
    if ((int32_t)(HAL_GetTick() - s_deadline) >= 0) {
        buzzer_stop();
    }
}
