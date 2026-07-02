#include "adc_shared.h"
#include "main.h"
#include "stm32f4xx_hal.h"

static ADC_HandleTypeDef s_hadc1;
static bool s_initialized = false;

void adc_shared_init(void)
{
    if (s_initialized) {
        return;
    }

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

    s_initialized = true;
}

bool adc_shared_read_channel(uint32_t channel, uint32_t sampling_time, uint16_t *raw)
{
    ADC_ChannelConfTypeDef ch = {0};

    if (raw == NULL) {
        return false;
    }

    adc_shared_init();

    ch.Channel      = channel;
    ch.Rank         = 1;
    ch.SamplingTime = sampling_time;
    ch.Offset       = 0;

    if (HAL_ADC_ConfigChannel(&s_hadc1, &ch) != HAL_OK) {
        return false;
    }

    if (HAL_ADC_Start(&s_hadc1) != HAL_OK) {
        return false;
    }

    if (HAL_ADC_PollForConversion(&s_hadc1, 10U) != HAL_OK) {
        (void)HAL_ADC_Stop(&s_hadc1);
        return false;
    }

    *raw = (uint16_t)HAL_ADC_GetValue(&s_hadc1);
    (void)HAL_ADC_Stop(&s_hadc1);
    return true;
}
