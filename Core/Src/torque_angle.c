#include "torque_angle.h"
#include "main.h"
#include "config.h"
#include "stm32f4xx_hal.h"

static ADC_HandleTypeDef s_hadc1;
static float s_zero_deg   = 0.0f;
static float s_target_deg = ANGLE_DEFAULT_DEG;

static float adc_read_absolute(void)
{
    if (HAL_ADC_Start(&s_hadc1) != HAL_OK) {
        return s_zero_deg;
    }

    if (HAL_ADC_PollForConversion(&s_hadc1, 10U) != HAL_OK) {
        (void)HAL_ADC_Stop(&s_hadc1);
        return s_zero_deg;
    }

    uint32_t raw = HAL_ADC_GetValue(&s_hadc1);
    (void)HAL_ADC_Stop(&s_hadc1);

    float v_pin    = ((float)raw / 4095.0f) * ANGLE_ADC_VREF;
    float v_sensor = v_pin / ANGLE_DIVIDER_RATIO;
    float deg      = (v_sensor / ANGLE_SENSOR_V_MAX) * 360.0f;

    if (deg < 0.0f)   deg = 0.0f;
    if (deg > 360.0f) deg = 360.0f;
    return deg;
}

void torque_angle_init(void)
{
    __HAL_RCC_ADC1_CLK_ENABLE();

    s_hadc1.Instance                   = ADC1;
    s_hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    s_hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    s_hadc1.Init.ScanConvMode          = DISABLE;
    s_hadc1.Init.ContinuousConvMode    = DISABLE;
    s_hadc1.Init.DiscontinuousConvMode = DISABLE;
    s_hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    s_hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    s_hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    s_hadc1.Init.NbrOfConversion       = 1;
    s_hadc1.Init.DMAContinuousRequests = DISABLE;
    s_hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;

    if (HAL_ADC_Init(&s_hadc1) != HAL_OK) {
        Error_Handler();
    }

    ADC_ChannelConfTypeDef ch = {0};
    ch.Channel      = ADC_CHANNEL_1;   // PA1
    ch.Rank         = 1;
    ch.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    ch.Offset       = 0;

    if (HAL_ADC_ConfigChannel(&s_hadc1, &ch) != HAL_OK) {
        Error_Handler();
    }

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

    if (delta < -180.0f) delta += 360.0f;
    if (delta >  180.0f) delta -= 360.0f;
    return (delta < 0.0f) ? 0.0f : delta;
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
    if (HAL_ADC_Start(&s_hadc1) != HAL_OK) {
        return 0U;
    }

    if (HAL_ADC_PollForConversion(&s_hadc1, 10U) != HAL_OK) {
        (void)HAL_ADC_Stop(&s_hadc1);
        return 0U;
    }

    uint16_t raw = (uint16_t)HAL_ADC_GetValue(&s_hadc1);
    (void)HAL_ADC_Stop(&s_hadc1);
    return raw;
}
