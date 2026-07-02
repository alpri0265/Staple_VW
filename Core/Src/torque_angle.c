#include "torque_angle.h"
#include "adc_shared.h"
#include "config.h"
#include "stm32f4xx_hal.h"

static float s_zero_deg   = 0.0f;
static float s_target_deg = ANGLE_DEFAULT_DEG;

static float adc_read_absolute(void)
{
    uint16_t raw = 0U;

    if (!adc_shared_read_channel(ADC_CHANNEL_1, ADC_SAMPLETIME_480CYCLES, &raw)) {
        return s_zero_deg;
    }

    float v_pin    = ((float)raw / 4095.0f) * ANGLE_ADC_VREF;
    float v_sensor = v_pin / ANGLE_DIVIDER_RATIO;
    float deg      = (v_sensor / ANGLE_SENSOR_V_MAX) * 360.0f;

    if (deg < 0.0f)   deg = 0.0f;
    if (deg > 360.0f) deg = 360.0f;
    return deg;
}

void torque_angle_init(void)
{
    adc_shared_init();
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
    uint16_t raw = 0U;
    if (!adc_shared_read_channel(ADC_CHANNEL_1, ADC_SAMPLETIME_480CYCLES, &raw)) {
        return 0U;
    }
    return raw;
}
