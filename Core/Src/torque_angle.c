#include "torque_angle.h"
#include "main.h"
#include "config.h"
#include "stm32f4xx_hal.h"

// PA1 = ADC1_CH1, через дільник 10kΩ/10kΩ від аналогового виходу датчика 0-5V

static ADC_HandleTypeDef s_hadc;
static float s_zero_deg   = 0.0f;
static float s_target_deg = ANGLE_DEFAULT_DEG;

static float adc_read_absolute(void)
{
    HAL_ADC_Start(&s_hadc);
    if (HAL_ADC_PollForConversion(&s_hadc, 10) != HAL_OK) {
        HAL_ADC_Stop(&s_hadc);
        return s_zero_deg;
    }
    uint32_t raw = HAL_ADC_GetValue(&s_hadc);
    HAL_ADC_Stop(&s_hadc);

    float v_pin    = ((float)raw / 4095.0f) * ANGLE_ADC_VREF;
    float v_sensor = v_pin / ANGLE_DIVIDER_RATIO;
    float deg      = (v_sensor / ANGLE_SENSOR_V_MAX) * 360.0f;
    if (deg < 0.0f)   deg = 0.0f;
    if (deg > 360.0f) deg = 360.0f;
    return deg;
}

void torque_angle_init(void)
{
    // GPIO PA1: аналоговий вхід
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin  = GPIO_PIN_1;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    // ADC1, канал 1 (PA1)
    __HAL_RCC_ADC1_CLK_ENABLE();

    s_hadc.Instance                   = ADC1;
    s_hadc.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    s_hadc.Init.Resolution            = ADC_RESOLUTION_12B;
    s_hadc.Init.ScanConvMode          = DISABLE;
    s_hadc.Init.ContinuousConvMode    = DISABLE;
    s_hadc.Init.DiscontinuousConvMode = DISABLE;
    s_hadc.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    s_hadc.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    s_hadc.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    s_hadc.Init.NbrOfConversion       = 1;
    s_hadc.Init.DMAContinuousRequests = DISABLE;
    s_hadc.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    HAL_ADC_Init(&s_hadc);

    ADC_ChannelConfTypeDef ch = {0};
    ch.Channel      = ADC_CHANNEL_1;
    ch.Rank         = 1;
    ch.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    HAL_ADC_ConfigChannel(&s_hadc, &ch);

    s_zero_deg = adc_read_absolute();
}

void torque_angle_zero(void)
{
    s_zero_deg = adc_read_absolute();
}

float torque_angle_get_deg(void)
{
    float abs_now = adc_read_absolute();
    float delta   = abs_now - s_zero_deg;
    // Обробка переходу 360°→0°
    if (delta < -180.0f) delta += 360.0f;
    if (delta >  180.0f) delta -= 360.0f;
    return delta < 0.0f ? 0.0f : delta;
}

void torque_angle_set_target(float deg)
{
    if (deg < 0.0f)          deg = 0.0f;
    if (deg > ANGLE_MAX_DEG) deg = ANGLE_MAX_DEG;
    s_target_deg = deg;
}

float torque_angle_get_target(void)
{
    return s_target_deg;
}

bool torque_angle_is_reached(void)
{
    return torque_angle_get_deg() >= s_target_deg;
}

uint16_t torque_angle_get_raw(void)
{
    HAL_ADC_Start(&s_hadc);
    if (HAL_ADC_PollForConversion(&s_hadc, 10) != HAL_OK) {
        HAL_ADC_Stop(&s_hadc);
        return 0;
    }
    uint16_t raw = (uint16_t)HAL_ADC_GetValue(&s_hadc);
    HAL_ADC_Stop(&s_hadc);
    return raw;
}
