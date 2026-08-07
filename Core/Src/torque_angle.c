#include "torque_angle.h"
#include "adc_shared.h"
#include "config.h"
#include "stm32f4xx_hal.h"

static float s_zero_deg     = 0.0f;
static float s_target_deg   = ANGLE_DEFAULT_DEG;
static float s_filtered_deg = -1.0f;  // <0 = фільтр ще не ініціалізований
static bool  s_reached_latched = false;

// Один сирий вимір АЦП → абсолютний кут (0..360), без фільтра.
static float adc_read_absolute_raw(void)
{
    uint16_t raw = 0U;

    if (!adc_shared_read_channel(ADC_CHANNEL_1, ADC_SAMPLETIME_480CYCLES, &raw)) {
        return (s_filtered_deg >= 0.0f) ? s_filtered_deg : s_zero_deg;
    }

    float v_pin    = ((float)raw / 4095.0f) * ANGLE_ADC_VREF;
    float v_sensor = v_pin / ANGLE_DIVIDER_RATIO;
    float deg      = (v_sensor / ANGLE_SENSOR_V_MAX) * 360.0f;

    if (deg < 0.0f)   deg = 0.0f;
    if (deg > 360.0f) deg = 360.0f;
    return deg;
}

// EMA-фільтрований абсолютний кут — прибирає тремтіння сирого АЦП.
static float adc_read_absolute(void)
{
    float deg = adc_read_absolute_raw();

    if (s_filtered_deg < 0.0f) {
        s_filtered_deg = deg;
    } else {
        s_filtered_deg += (deg - s_filtered_deg) * ANGLE_FILTER_ALPHA;
    }
    return s_filtered_deg;
}

void torque_angle_init(void)
{
    adc_shared_init();

    // Прогріваємо фільтр кількома зразками, щоб перший "нуль" не був сирим одиничним виміром.
    s_filtered_deg = -1.0f;
    for (uint8_t i = 0; i < 8; i++) {
        (void)adc_read_absolute();
    }
    s_zero_deg = s_filtered_deg;
    s_reached_latched = false;
}

void torque_angle_zero(void)
{
    s_zero_deg = adc_read_absolute();
    s_reached_latched = false;
}

// Кут дотяжки монотонно накопичується від нуля (0..360, максимум один повний оберт) —
// це НЕ задача "найкоротша відстань між кутами", тому старе обгортання ±180°
// тут неправильне: воно давало розрив (стрибок на 0) щоразу, коли реальний кут
// наближався до 180°, бо будь-яке delta>180 підмінялось на delta-360 (від'ємне) і
// одразу обрізалось нулем. Тепер обгортаємо лише "справжній" перехід через межу
// датчика 360°/0° (delta глибоко від'ємне), а невеликий шум біля нуля просто кліпаємо.
float torque_angle_get_deg(void)
{
    float abs_now = adc_read_absolute();
    float delta   = abs_now - s_zero_deg;

    if (delta < -180.0f) {
        delta += 360.0f;   // сенсор реально перейшов позначку 360°/0° під час дотяжки
    } else if (delta < 0.0f) {
        delta = 0.0f;       // невеликий шум навколо точки нуля — не обгортання
    }
    return delta;
}

void torque_angle_set_target(float deg)
{
    if (deg < 0.0f)          deg = 0.0f;
    if (deg > ANGLE_MAX_DEG) deg = ANGLE_MAX_DEG;
    s_target_deg = deg;
    s_reached_latched = false;
}

float torque_angle_get_target(void)
{
    return s_target_deg;
}

// Гістерезис: раз спрацювавши, лишається "reached" поки кут не впаде
// нижче (ціль - ANGLE_REACHED_HYSTERESIS_DEG) — без цього шум АЦП
// біля порогу змушував прапор (і індикацію DONE! на LCD) тремтіти.
bool torque_angle_is_reached(void)
{
    float deg = torque_angle_get_deg();

    if (!s_reached_latched) {
        if (deg >= s_target_deg) s_reached_latched = true;
    } else {
        if (deg < (s_target_deg - ANGLE_REACHED_HYSTERESIS_DEG)) s_reached_latched = false;
    }

    return s_reached_latched;
}

uint16_t torque_angle_get_raw(void)
{
    uint16_t raw = 0U;
    if (!adc_shared_read_channel(ADC_CHANNEL_1, ADC_SAMPLETIME_480CYCLES, &raw)) {
        return 0U;
    }
    return raw;
}
