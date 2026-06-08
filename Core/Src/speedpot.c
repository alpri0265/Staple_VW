#include "speedpot.h"
#include "main.h"
#include "config.h"
#include "stm32f4xx_hal.h"

// ADC2, Channel 2 (PA2) — потенціометр швидкості
// Потенціометр: один кінець 3.3V, інший GND, середній контакт → PA2

static ADC_HandleTypeDef s_hadc2;

void speedpot_init(void)
{
    __HAL_RCC_ADC2_CLK_ENABLE();

    s_hadc2.Instance                   = ADC2;
    s_hadc2.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    s_hadc2.Init.Resolution            = ADC_RESOLUTION_12B;
    s_hadc2.Init.ScanConvMode          = DISABLE;
    s_hadc2.Init.ContinuousConvMode    = DISABLE;
    s_hadc2.Init.DiscontinuousConvMode = DISABLE;
    s_hadc2.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    s_hadc2.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    s_hadc2.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    s_hadc2.Init.NbrOfConversion       = 1;
    s_hadc2.Init.DMAContinuousRequests = DISABLE;
    s_hadc2.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    HAL_ADC_Init(&s_hadc2);

    ADC_ChannelConfTypeDef ch = {0};
    ch.Channel      = ADC_CHANNEL_2;   // PA2
    ch.Rank         = 1;
    ch.SamplingTime = ADC_SAMPLETIME_56CYCLES;
    HAL_ADC_ConfigChannel(&s_hadc2, &ch);
}

uint16_t speedpot_get_speed(void)
{
    HAL_ADC_Start(&s_hadc2);
    if (HAL_ADC_PollForConversion(&s_hadc2, 5) != HAL_OK) {
        HAL_ADC_Stop(&s_hadc2);
        return SPEED_FAST;
    }
    uint32_t raw = HAL_ADC_GetValue(&s_hadc2);
    HAL_ADC_Stop(&s_hadc2);

    // raw 0..4095 → SPEED_POT_MIN..SPEED_POT_MAX
    uint32_t speed = SPEED_POT_MIN +
                     (raw * (uint32_t)(SPEED_POT_MAX - SPEED_POT_MIN)) / 4095U;
    return (uint16_t)speed;
}
