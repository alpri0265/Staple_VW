#include "speedpot.h"
#include "main.h"
#include "stm32f4xx_hal.h"

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

    if (HAL_ADC_Init(&s_hadc2) != HAL_OK) {
        Error_Handler();
    }

    ADC_ChannelConfTypeDef ch = {0};
    ch.Channel      = ADC_CHANNEL_2;   // PA2
    ch.Rank         = 1;
    ch.SamplingTime = ADC_SAMPLETIME_56CYCLES;
    ch.Offset       = 0;

    if (HAL_ADC_ConfigChannel(&s_hadc2, &ch) != HAL_OK) {
        Error_Handler();
    }
}

uint16_t speedpot_get_raw(void)
{
    if (HAL_ADC_Start(&s_hadc2) != HAL_OK) {
        return 0U;
    }

    if (HAL_ADC_PollForConversion(&s_hadc2, 5U) != HAL_OK) {
        (void)HAL_ADC_Stop(&s_hadc2);
        return 0U;
    }

    uint32_t raw = HAL_ADC_GetValue(&s_hadc2);
    (void)HAL_ADC_Stop(&s_hadc2);
    return (uint16_t)raw;
}
