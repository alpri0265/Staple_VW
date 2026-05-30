#include "torque_angle.h"
#include "main.h"
#include "config.h"
#include "stm32f4xx_hal.h"

// ===== Стан модуля =====

static TIM_HandleTypeDef s_htim3;
static uint16_t          s_zero_count  = 32768U;  // значення лічильника при нулі
static float             s_target_deg  = ANGLE_DEFAULT_DEG;

// ===== Приватні функції =====

static int32_t get_signed_count(void)
{
    uint16_t raw = (uint16_t)__HAL_TIM_GET_COUNTER(&s_htim3);
    // Різниця зі знаком: позитивна = за годинниковою
    return (int32_t)raw - (int32_t)s_zero_count;
}

// ===== Публічні функції =====

void torque_angle_init(void)
{
    // --- GPIO: PC6 (TIM3_CH1 = A) та PC7 (TIM3_CH2 = B) ---
    // GPIOC clock вже увімкнено в MX_GPIO_Init (для HX711)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin       = ANGLE_A_Pin | ANGLE_B_Pin;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;   // NPN open-collector encoder
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(ANGLE_A_GPIO_Port, &GPIO_InitStruct);

    // --- TIM3 у режимі Encoder Interface ---
    __HAL_RCC_TIM3_CLK_ENABLE();

    s_htim3.Instance               = TIM3;
    s_htim3.Init.Prescaler         = 0;
    s_htim3.Init.CounterMode       = TIM_COUNTERMODE_UP;
    s_htim3.Init.Period            = 0xFFFFU;   // 16-bit максимум
    s_htim3.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    s_htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    TIM_Encoder_InitTypeDef encoder = {0};
    encoder.EncoderMode   = TIM_ENCODERMODE_TI12;      // обидва канали (4x = 1440 кроків/об)
    encoder.IC1Polarity   = TIM_ICPOLARITY_RISING;
    encoder.IC1Selection  = TIM_ICSELECTION_DIRECTTI;
    encoder.IC1Prescaler  = TIM_ICPSC_DIV1;
    encoder.IC1Filter     = 0x06;  // цифровий фільтр ~192 нс @ 168 МГц
    encoder.IC2Polarity   = TIM_ICPOLARITY_RISING;
    encoder.IC2Selection  = TIM_ICSELECTION_DIRECTTI;
    encoder.IC2Prescaler  = TIM_ICPSC_DIV1;
    encoder.IC2Filter     = 0x06;

    HAL_TIM_Encoder_Init(&s_htim3, &encoder);

    // Центруємо лічильник для симетричного відліку
    __HAL_TIM_SET_COUNTER(&s_htim3, 32768U);
    s_zero_count = 32768U;

    HAL_TIM_Encoder_Start(&s_htim3, TIM_CHANNEL_ALL);
}

void torque_angle_zero(void)
{
    s_zero_count = (uint16_t)__HAL_TIM_GET_COUNTER(&s_htim3);
}

float torque_angle_get_deg(void)
{
    int32_t delta = get_signed_count();
    return (float)delta * ANGLE_DEG_PER_STEP;
}

void torque_angle_set_target(float deg)
{
    if (deg < 0.0f)            deg = 0.0f;
    if (deg > ANGLE_MAX_DEG)   deg = ANGLE_MAX_DEG;
    s_target_deg = deg;
}

float torque_angle_get_target(void)
{
    return s_target_deg;
}

bool torque_angle_is_reached(void)
{
    float current = torque_angle_get_deg();
    // Порівнюємо абсолютне значення: затяжка може бути в будь-який бік
    if (current < 0.0f) current = -current;
    return current >= s_target_deg;
}

uint16_t torque_angle_get_raw(void)
{
    return (uint16_t)__HAL_TIM_GET_COUNTER(&s_htim3);
}
