#include "speedpot.h"
#include "adc_shared.h"
#include "stm32f4xx_hal.h"

void speedpot_init(void)
{
    adc_shared_init();
}

uint16_t speedpot_get_raw(void)
{
    uint16_t raw = 0U;
    if (!adc_shared_read_channel(ADC_CHANNEL_2, ADC_SAMPLETIME_56CYCLES, &raw)) {
        return 0U;
    }
    return raw;
}
