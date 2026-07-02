#ifndef __ADC_SHARED_H
#define __ADC_SHARED_H

#include <stdbool.h>
#include <stdint.h>

void adc_shared_init(void);
bool adc_shared_read_channel(uint32_t channel, uint32_t sampling_time, uint16_t *raw);

#endif /* __ADC_SHARED_H */
